#include "services/SimulationEngine.h"
#include "models/Room.h"
#include "structures/LinkedQueue.h"
#include "structures/BinaryMinHeap.h"
#include <iostream>
#include <iomanip>

SimulationEngine::SimulationEngine(Hospital* h, const SimulationConfig& cfg, LogMode mode)
    : hospital(h), config(cfg), currentMinute(0), masterEvents(nullptr), masterEventCount(0), logMode(mode) {
    statsService.reset();
}

void SimulationEngine::setLogMode(LogMode mode) {
    logMode = mode;
}

void SimulationEngine::updateMasterPatient(const std::string& id, PatientStatus status, int waitMin, int startMin) {
    if (masterEvents == nullptr || masterEventCount <= 0) return;
    for (int i = 0; i < masterEventCount; ++i) {
        if (masterEvents[i].id == id) {
            masterEvents[i].status = status;
            if (waitMin >= 0) masterEvents[i].waitMinutes = waitMin;
            if (startMin >= 0) masterEvents[i].serviceStartMinute = startMin;
            break;
        }
    }
}

void SimulationEngine::runSimulation(Patient inputEvents[], int eventCount) {
    statsService.reset();
    currentMinute = 0;
    masterEvents = inputEvents;
    masterEventCount = eventCount;

    if (hospital == nullptr) return;

    // Đưa trạng thái các phòng về ban đầu trước khi bắt đầu mô phỏng
    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();
    for (int i = 0; i < roomCount; ++i) {
        if (rooms[i] != nullptr) {
            rooms[i]->getRegularQueue().clear();
            rooms[i]->getPriorityHeap().clear();
            if (rooms[i]->isBusy()) {
                rooms[i]->finishCurrentService();
            }
        }
    }

    // Vòng lặp thời gian chính chạy từ t = 0 đến totalSimulationMinutes - 1
    for (currentMinute = 0; currentMinute < config.totalSimulationMinutes; ++currentMinute) {
        // 1. Tiếp nhận và phân phòng cho bệnh nhân đến tại phút hiện tại
        processArrivals(inputEvents, eventCount);

        // 2. Xử lý bệnh nhân bỏ về do chờ quá ngưỡng T phút
        processAbandonment();

        // 3. Cập nhật Aging (tăng mức ưu tiên hiệu lực và chuyển từ DSLK sang Heap)
        processAging();

        // 4. Cập nhật tiến độ khám và gọi bệnh nhân tiếp theo cho phòng rảnh
        processRoomTick();

        // Cập nhật độ dài hàng đợi lớn nhất tại phút hiện tại
        for (int i = 0; i < roomCount; ++i) {
            if (rooms[i] != nullptr) {
                int qLen = rooms[i]->getRegularQueue().size() + rooms[i]->getPriorityHeap().size();
                statsService.updateMaxQueueLength(qLen);
            }
        }

        // 5. In trạng thái các phòng nếu chọn chế độ theo dõi từng phút
        if (logMode == LogMode::MINUTE_TICK) {
            printMinuteStatus();
        }
    }

    // Đảm bảo cập nhật maxQueueLength từ tất cả các phòng
    for (int i = 0; i < roomCount; ++i) {
        if (rooms[i] != nullptr) {
            statsService.updateMaxQueueLength(rooms[i]->getMaxQueueLength());
        }
    }
}

void SimulationEngine::processArrivals(Patient arrivals[], int count) {
    if (hospital == nullptr || arrivals == nullptr) return;

    int avgService = (config.minServiceDuration + config.maxServiceDuration) / 2;

    for (int i = 0; i < count; ++i) {
        if (arrivals[i].arrivalMinute == currentMinute) {
            Patient p = arrivals[i];
            int basePrio = static_cast<int>(p.priority);

            p.effectivePriority = basePrio;
            p.waitMinutes = 0;
            p.serviceStartMinute = -1;
            p.status = PatientStatus::WAITING;

            // Tìm phòng phù hợp chuyên khoa có thời gian chờ ước lượng thấp nhất
            Room* targetRoom = hospital->selectBestRoom(p.specialty, avgService);

            if (targetRoom == nullptr) {
                p.status = PatientStatus::REJECTED;
                arrivals[i].status = PatientStatus::REJECTED;
                statsService.recordRejected(basePrio);
                if (logMode == LogMode::EVENTS) {
                    std::cout << "  [Phút " << std::setw(3) << currentMinute << "] TỪ CHỐI TIẾP NHẬN: Bệnh nhân "
                              << p.id << " (" << p.fullName << ") do không có phòng khám chuyên khoa " << p.specialty << ".\n";
                }
            } else {
                statsService.recordArrival(basePrio);
                targetRoom->addPatient(p, config.priorityEnabled);
                if (logMode == LogMode::EVENTS) {
                    bool inHeap = config.priorityEnabled && (p.priority != PriorityLevel::NORMAL);
                    std::cout << "  [Phút " << std::setw(3) << currentMinute << "] Tiếp nhận: "
                              << p.id << " (" << p.fullName << " - " << StatisticsService::priorityNameVi(basePrio)
                              << ") ➔ Phân vào Phòng " << targetRoom->getRoomId() << " ("
                              << (inHeap ? "Hàng ưu tiên Heap" : "Hàng thường DSLK") << ").\n";
                }
            }
        }
    }
}

void SimulationEngine::processAbandonment() {
    if (hospital == nullptr || config.waitThresholdT <= 0) return;

    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();

    for (int i = 0; i < roomCount; ++i) {
        Room* r = rooms[i];
        if (r == nullptr) continue;

        // 1. Kiểm tra hàng đợi thường (LinkedQueue)
        LinkedQueue& regQueue = r->getRegularQueue();
        int regSize = regQueue.size();
        for (int k = 0; k < regSize; ++k) {
            Patient p;
            if (regQueue.dequeue(p)) {
                int waited = currentMinute - p.arrivalMinute;
                if (waited > config.waitThresholdT) {
                    p.waitMinutes = waited;
                    p.status = PatientStatus::LEFT;
                    statsService.recordLeft(static_cast<int>(p.priority));
                    updateMasterPatient(p.id, PatientStatus::LEFT, waited, -1);
                    if (logMode == LogMode::EVENTS) {
                        std::cout << "  [Phút " << std::setw(3) << currentMinute << "] BỎ VỀ: Bệnh nhân "
                                  << p.id << " (" << p.fullName << ") rời hàng chờ Phòng " << r->getRoomId()
                                  << " do chờ quá " << waited << " phút (vượt ngưỡng T = " << config.waitThresholdT << " phút).\n";
                    }
                } else {
                    regQueue.enqueue(p); // Chưa quá T, giữ nguyên thứ tự FIFO
                }
            }
        }

        // 2. Kiểm tra hàng đợi ưu tiên (BinaryMinHeap)
        BinaryMinHeap& prioHeap = r->getPriorityHeap();
        int heapSize = prioHeap.size();
        if (heapSize > 0) {
            Patient* rawArr = prioHeap.getRawArray();
            std::string* idsToRemove = new std::string[heapSize];
            int removeCount = 0;

            for (int idx = 0; idx < heapSize; ++idx) {
                int waited = currentMinute - rawArr[idx].arrivalMinute;
                if (waited > config.waitThresholdT) {
                    idsToRemove[removeCount++] = rawArr[idx].id;
                }
            }

            for (int idx = 0; idx < removeCount; ++idx) {
                Patient removedP;
                if (prioHeap.removeById(idsToRemove[idx], removedP)) {
                    int waited = currentMinute - removedP.arrivalMinute;
                    removedP.waitMinutes = waited;
                    removedP.status = PatientStatus::LEFT;
                    statsService.recordLeft(static_cast<int>(removedP.priority));
                    updateMasterPatient(removedP.id, PatientStatus::LEFT, waited, -1);
                    if (logMode == LogMode::EVENTS) {
                        std::cout << "  [Phút " << std::setw(3) << currentMinute << "] BỎ VỀ: Bệnh nhân "
                                  << removedP.id << " (" << removedP.fullName << ") rời hàng ưu tiên Phòng " << r->getRoomId()
                                  << " do chờ quá " << waited << " phút (vượt ngưỡng T = " << config.waitThresholdT << " phút).\n";
                    }
                }
            }

            delete[] idsToRemove;
        }
    }
}

void SimulationEngine::processAging() {
    if (hospital == nullptr || !config.agingEnabled || config.agingInterval <= 0) return;

    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();

    for (int i = 0; i < roomCount; ++i) {
        Room* r = rooms[i];
        if (r == nullptr) continue;

        LinkedQueue& regQueue = r->getRegularQueue();
        BinaryMinHeap& prioHeap = r->getPriorityHeap();

        // 1. Cập nhật Aging cho bệnh nhân trong Heap
        int heapSize = prioHeap.size();
        Patient* rawArr = prioHeap.getRawArray();
        bool heapChanged = false;

        for (int idx = 0; idx < heapSize; ++idx) {
            int waited = currentMinute - rawArr[idx].arrivalMinute;
            if (waited > 0 && waited % config.agingInterval == 0) {
                if (rawArr[idx].effectivePriority > static_cast<int>(PriorityLevel::EMERGENCY)) {
                    rawArr[idx].effectivePriority--;
                    heapChanged = true;
                    if (logMode == LogMode::EVENTS) {
                        std::cout << "  [Phút " << std::setw(3) << currentMinute << "] TĂNG ƯU TIÊN (Aging): Bệnh nhân "
                                  << rawArr[idx].id << " chờ " << waited << " phút ➔ Nâng mức ưu tiên lên Cấp "
                                  << rawArr[idx].effectivePriority << " (Heap Phòng " << r->getRoomId() << ").\n";
                    }
                }
            }
        }
        if (heapChanged) {
            prioHeap.buildHeap(); // Tái lập Min-Heap bottom-up O(P)
        }

        // 2. Cập nhật Aging cho bệnh nhân ở hàng thường (DSLK) -> chuyển sang Heap nếu đủ mức
        int regSize = regQueue.size();
        for (int k = 0; k < regSize; ++k) {
            Patient p;
            if (regQueue.dequeue(p)) {
                int waited = currentMinute - p.arrivalMinute;
                if (waited > 0 && waited % config.agingInterval == 0) {
                    if (p.effectivePriority > static_cast<int>(PriorityLevel::EMERGENCY)) {
                        p.effectivePriority--;
                    }
                    if (config.priorityEnabled && p.effectivePriority < static_cast<int>(PriorityLevel::NORMAL)) {
                        prioHeap.insert(p);
                        if (logMode == LogMode::EVENTS) {
                            std::cout << "  [Phút " << std::setw(3) << currentMinute << "] TĂNG ƯU TIÊN (Aging): Bệnh nhân "
                                      << p.id << " (" << p.fullName << ") chờ " << waited << " phút ➔ Chuyển từ DSLK sang Heap Phòng "
                                      << r->getRoomId() << " (Cấp " << p.effectivePriority << ").\n";
                        }
                        continue; // Đã chuyển sang Heap, không đẩy lại DSLK
                    }
                }
                regQueue.enqueue(p);
            }
        }
    }
}

void SimulationEngine::processRoomTick() {
    if (hospital == nullptr) return;

    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();

    for (int i = 0; i < roomCount; ++i) {
        Room* r = rooms[i];
        if (r == nullptr) continue;

        // 1. Nếu phòng đang bận, giảm 1 phút khám; xong thì giải phóng phòng
        if (r->isBusy()) {
            r->updateServiceTick();
            if (r->isServiceComplete()) {
                if (r->getCurrentPatient() != nullptr) {
                    if (logMode == LogMode::EVENTS) {
                        std::cout << "  [Phút " << std::setw(3) << currentMinute << "] Khám xong: Phòng "
                                  << r->getRoomId() << " hoàn tất ca khám của "
                                  << r->getCurrentPatient()->id << " (" << r->getCurrentPatient()->fullName << ").\n";
                    }
                    updateMasterPatient(r->getCurrentPatient()->id, PatientStatus::COMPLETED,
                                        r->getCurrentPatient()->waitMinutes,
                                        r->getCurrentPatient()->serviceStartMinute);
                }
                r->finishCurrentService();
            }
        }

        // 2. Nếu phòng rảnh, gọi bệnh nhân tiếp theo từ hàng đợi
        if (!r->isBusy()) {
            Patient* nextP = r->fetchNextPatient(config.priorityEnabled);
            if (nextP != nullptr) {
                nextP->serviceStartMinute = currentMinute;
                nextP->waitMinutes = currentMinute - nextP->arrivalMinute;
                nextP->status = PatientStatus::IN_SERVICE;

                statsService.recordServed(static_cast<int>(nextP->priority), nextP->waitMinutes);
                updateMasterPatient(nextP->id, PatientStatus::IN_SERVICE, nextP->waitMinutes, currentMinute);
                if (logMode == LogMode::EVENTS) {
                    std::cout << "  [Phút " << std::setw(3) << currentMinute << "] Bắt đầu khám: Phòng "
                              << r->getRoomId() << " mời " << nextP->id << " (" << nextP->fullName
                              << ", STT " << nextP->ticketNumber << ", dự kiến " << nextP->serviceDuration << " phút).\n";
                }
                r->startService(nextP, currentMinute);
            }
        }
    }
}

void SimulationEngine::printMinuteStatus() const {
    if (hospital == nullptr) return;

    std::cout << "  [Phút " << std::setw(3) << currentMinute << "] ";
    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();

    for (int i = 0; i < roomCount; ++i) {
        Room* r = rooms[i];
        if (r == nullptr) continue;

        std::cout << r->getRoomId() << " (" << r->getSpecialty() << "): ";
        if (r->isBusy() && r->getCurrentPatient() != nullptr) {
            std::cout << "Đang khám(" << r->getCurrentPatient()->id
                      << ", còn " << r->getRemainingServiceTime() << "p)";
        } else {
            std::cout << "Rảnh";
        }
        std::cout << " [Heap:" << r->getPriorityHeap().size()
                  << ", DSLK:" << r->getRegularQueue().size() << "]";
        if (i < roomCount - 1) std::cout << " │ ";
    }
    std::cout << "\n";
}

const StatisticsService& SimulationEngine::getStatisticsService() const {
    return statsService;
}
