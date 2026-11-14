#include "services/SimulationEngine.h"
#include "models/Room.h"
#include "structures/LinkedQueue.h"
#include "structures/BinaryMinHeap.h"
#include <iostream>
#include <iomanip>

SimulationEngine::SimulationEngine(Hospital* h, const SimulationConfig& cfg)
    : hospital(h), config(cfg), currentMinute(0) {
    statsService.reset();
}

void SimulationEngine::runSimulation(Patient inputEvents[], int eventCount) {
    statsService.reset();
    currentMinute = 0;

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

    // Vòng lặp thời gian chính chạy từ t = 0 đến totalSimulationMinutes - 1[cite: 2, 4]
    for (currentMinute = 0; currentMinute < config.totalSimulationMinutes; ++currentMinute) {
        // 1. Tiếp nhận và phân phòng cho bệnh nhân đến tại phút hiện tại[cite: 2, 4]
        processArrivals(inputEvents, eventCount);

        // 2. Xử lý bệnh nhân bỏ về do chờ quá ngưỡng T phút[cite: 2, 4]
        processAbandonment();

        // 3. Cập nhật Aging (tăng mức ưu tiên hiệu lực và chuyển từ DSLK sang Heap)[cite: 2, 4]
        processAging();

        // 4. Cập nhật tiến độ khám và gọi bệnh nhân tiếp theo cho phòng rảnh[cite: 2, 4]
        processRoomTick();

        // 5. In trạng thái tóm tắt các phòng tại phút hiện tại[cite: 2, 4]
        printMinuteStatus();
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
                statsService.recordRejected(basePrio);
            } else {
                statsService.recordArrival(basePrio);
                targetRoom->addPatient(p, config.priorityEnabled);
            }
        }
    }
}

void SimulationEngine::processAbandonment() {
    if (hospital == nullptr || config.waitThresholdT < 0) return;

    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();

    for (int i = 0; i < roomCount; ++i) {
        Room* r = rooms[i];
        if (r == nullptr) continue;

        // 1. Kiểm tra hàng đợi thường (LinkedQueue)[cite: 2, 3]
        LinkedQueue& regQueue = r->getRegularQueue();
        int regSize = regQueue.size();
        for (int k = 0; k < regSize; ++k) {
            Patient p;
            if (regQueue.dequeue(p)) {
                int waited = currentMinute - p.arrivalMinute;
                if (waited > config.waitThresholdT) {
                    p.waitMinutes = waited;
                    p.status = PatientStatus::LEFT;
                } else {
                    regQueue.enqueue(p); // Chưa quá T, giữ nguyên thứ tự FIFO[cite: 2]
                }
            }
        }

        // 2. Kiểm tra hàng đợi ưu tiên (BinaryMinHeap)[cite: 2, 3]
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
                    removedP.waitMinutes = currentMinute - removedP.arrivalMinute;
                    removedP.status = PatientStatus::LEFT;
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

        // 1. Cập nhật Aging cho bệnh nhân trong Heap[cite: 2, 3]
        int heapSize = prioHeap.size();
        Patient* rawArr = prioHeap.getRawArray();
        bool heapChanged = false;

        for (int idx = 0; idx < heapSize; ++idx) {
            int waited = currentMinute - rawArr[idx].arrivalMinute;
            if (waited > 0 && waited % config.agingInterval == 0) {
                if (rawArr[idx].effectivePriority > static_cast<int>(PriorityLevel::EMERGENCY)) {
                    rawArr[idx].effectivePriority--;
                    heapChanged = true;
                }
            }
        }
        if (heapChanged) {
            prioHeap.buildHeap(); // Tái lập Min-Heap bottom-up O(P)[cite: 3, 4]
        }

        // 2. Cập nhật Aging cho bệnh nhân ở hàng thường (DSLK) -> chuyển sang Heap nếu đủ mức[cite: 2, 4]
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
                        continue; // Đã chuyển sang Heap, không đẩy lại DSLK[cite: 2]
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

        // 1. Nếu phòng đang bận, giảm 1 phút khám; xong thì giải phóng phòng[cite: 2, 3]
        if (r->isBusy()) {
            r->updateServiceTick();
            if (!r->isBusy()) {
                r->finishCurrentService();
            }
        }

        // 2. Nếu phòng rảnh, gọi bệnh nhân tiếp theo từ hàng đợi[cite: 2, 3]
        if (!r->isBusy()) {
            Patient* nextP = r->fetchNextPatient(config.priorityEnabled);
            if (nextP != nullptr) {
                nextP->serviceStartMinute = currentMinute;
                nextP->waitMinutes = currentMinute - nextP->arrivalMinute;
                nextP->status = PatientStatus::IN_SERVICE;

                // Ghi nhận thống kê khi bệnh nhân được gọi khám (dùng mức ưu tiên gốc)
                statsService.recordServed(static_cast<int>(nextP->priority), nextP->waitMinutes);
                r->startService(nextP, currentMinute);
            }
        }
    }
}

void SimulationEngine::printMinuteStatus() const {
    if (hospital == nullptr) return;

    std::cout << "[Phut " << std::setw(3) << currentMinute << "] ";
    int roomCount = hospital->getRoomCount();
    Room** rooms = hospital->getRooms();

    for (int i = 0; i < roomCount; ++i) {
        Room* r = rooms[i];
        if (r == nullptr) continue;

        std::cout << r->getRoomId() << "(" << r->getSpecialty() << "): "
                  << (r->isBusy() ? "Dang kham" : "Ranh")
                  << " [Heap:" << r->getPriorityHeap().size()
                  << ", Thuong:" << r->getRegularQueue().size() << "] | ";
    }
    std::cout << "\n";
}

const StatisticsService& SimulationEngine::getStatisticsService() const {
    return statsService;
}