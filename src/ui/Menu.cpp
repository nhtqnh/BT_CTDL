#include "ui/Menu.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>

#include "algorithms/SortUtils.h"
#include "services/FileManager.h"
#include "services/StatisticsService.h"
#include "services/SimulationEngine.h"

namespace {
    const char* kDefaultPatientsPath = "data/patients_sample.txt";
    const char* kDefaultLargePath = "data/patients_large.txt";
    const char* kDefaultConfigPath = "data/config.txt";
    const char* kDefaultResultPath = "results/simulation_results.txt";

    const std::string kSpecialties[] = {
        "Nội khoa", "Ngoại khoa", "Nhi khoa", "Sản phụ khoa",
        "Nhãn khoa", "Da liễu", "Tai mũi họng", "Răng hàm mặt"
    };
    const int kSpecialtyCount = 8;

    std::string trimText(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
        while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
        return s.substr(a, b - a);
    }

    bool parseLong(const std::string& text, long long& out) {
        if (text.empty()) return false;
        try {
            size_t pos = 0;
            long long v = std::stoll(text, &pos);
            if (pos != text.size()) return false;
            out = v;
            return true;
        }
        catch (...) {
            return false;
        }
    }

    int utf8VisualWidth(const std::string& str) {
        int count = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            if ((static_cast<unsigned char>(str[i]) & 0xC0) != 0x80) {
                count++;
            }
        }
        return count;
    }

    void printUtf8Left(std::ostream& os, const std::string& str, int targetWidth) {
        int vWidth = utf8VisualWidth(str);
        os << str;
        if (targetWidth > vWidth) {
            os << std::string(targetWidth - vWidth, ' ');
        }
    }

    void printUtf8Right(std::ostream& os, const std::string& str, int targetWidth) {
        int vWidth = utf8VisualWidth(str);
        if (targetWidth > vWidth) {
            os << std::string(targetWidth - vWidth, ' ');
        }
        os << str;
    }
}

Menu::Menu()
    : patientsPath_(kDefaultPatientsPath),
      liveHospital_(nullptr),
      nextTicketNumber_(1),
      hasResult_(false) {
    config_.totalSimulationMinutes = 480;
    config_.numberOfRooms = 6;
    config_.minServiceDuration = 5;
    config_.maxServiceDuration = 20;
    config_.randomSeed = 12345;
    config_.priorityEnabled = true;
    config_.agingEnabled = false;
    config_.agingInterval = 10;
    config_.waitThresholdT = 60;

    liveHospital_ = new Hospital();
    initLiveHospital();
}

Menu::~Menu() {
    delete liveHospital_;
    liveHospital_ = nullptr;
}

void Menu::initLiveHospital() {
    if (liveHospital_ == nullptr) {
        liveHospital_ = new Hospital();
    }
    liveHospital_->clearRooms();
    liveHospital_->createRooms(config_.numberOfRooms, kSpecialties, kSpecialtyCount);
}

// ─── Nhập liệu an toàn (Chống kẹt phím, hỗ trợ giá trị mặc định) ─────────────

bool Menu::readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    std::cout.flush();
    if (!std::getline(std::cin, out)) return false;
    out = trimText(out);
    return true;
}

bool Menu::readInt(const std::string& prompt, int minV, int maxV, int& out) {
    while (true) {
        std::string line;
        if (!readLine(prompt, line)) return false;
        if (line.empty() && out >= minV && out <= maxV) {
            return true;
        }
        long long v = 0;
        if (parseLong(line, v) && v >= minV && v <= maxV) {
            out = static_cast<int>(v);
            return true;
        }
        std::cout << "  Giá trị không hợp lệ. Vui lòng nhập số nguyên từ "
                  << minV << " đến " << maxV << ".\n";
    }
}

bool Menu::readIntKeep(const std::string& prompt, int minV, int maxV, int& value) {
    while (true) {
        std::string line;
        if (!readLine(prompt + " [" + std::to_string(value) + "]: ", line)) return false;
        if (line.empty()) return true;
        long long v = 0;
        if (parseLong(line, v) && v >= minV && v <= maxV) {
            value = static_cast<int>(v);
            return true;
        }
        std::cout << "  Giá trị không hợp lệ. Vui lòng nhập từ "
                  << minV << " đến " << maxV << " (hoặc Enter để giữ nguyên).\n";
    }
}

bool Menu::readBoolKeep(const std::string& prompt, bool& value) {
    while (true) {
        std::string line;
        if (!readLine(prompt + " (y/n) [" + (value ? "y" : "n") + "]: ", line)) return false;
        if (line.empty()) return true;
        if (line == "y" || line == "Y" || line == "1") { value = true;  return true; }
        if (line == "n" || line == "N" || line == "0") { value = false; return true; }
        std::cout << "  Vui lòng chỉ nhập 'y' (có) hoặc 'n' (không).\n";
    }
}

bool Menu::readPath(const std::string& prompt,
    const std::string& defaultPath, std::string& out) {
    std::string line;
    if (!readLine(prompt + " [" + defaultPath + "]: ", line)) return false;
    out = line.empty() ? defaultPath : line;
    return true;
}

// ─── Hiển thị Menu chính ───────────────────────────────────────────────────

void Menu::printMenu() const {
    std::cout << "\n"
        << "┌────────────────────────────────────────────────────────────────────────┐\n"
        << "│           HỆ THỐNG ĐIỀU PHỐI VÀ QUẢN LÝ PHÒNG KHÁM BỆNH VIỆN           │\n"
        << "├────────────────────────────────────────────────────────────────────────┤\n"
        << "│ Bệnh nhân: " << std::right << std::setw(4) << patients_.size() << " ca │ "
        << "Phòng khám: " << std::right << std::setw(2) << config_.numberOfRooms << " phòng │ "
        << "Ưu tiên: " << (config_.priorityEnabled ? "BẬT" : "TẮT") << " │ "
        << "Aging: " << (config_.agingEnabled ? "BẬT" : "TẮT") << "    │\n"
        << "└────────────────────────────────────────────────────────────────────────┘\n\n"
        << " [1] Điều phối bệnh nhân\n"
        << "     1. Tiếp nhận bệnh nhân mới (Cấp STT & phân phòng)\n"
        << "     2. Gọi bệnh nhân kế tiếp vào khám\n"
        << "     3. Xem trạng thái phòng khám & hàng đợi trực tiếp\n\n"
        << " [2] Mô phỏng & So sánh\n"
        << "     4. Mô phỏng ngày làm việc (Tóm tắt / Sự kiện / Từng phút)\n"
        << "     5. So sánh các kịch bản vận hành\n\n"
        << " [3] Thống kê & Báo cáo\n"
        << "     6. Báo cáo thống kê hiệu năng\n\n"
        << " [4] Quản lý dữ liệu & Cấu hình\n"
        << "     7. Nạp danh sách bệnh nhân từ tệp\n"
        << "     8. Tạo dữ liệu bệnh nhân ngẫu nhiên\n"
        << "     9. Thiết lập cấu hình hệ thống\n"
        << "    10. Xuất kết quả mô phỏng ra tệp\n\n"
        << "     0. Thoát chương trình\n"
        << "──────────────────────────────────────────────────────────────────────────\n";
}

void Menu::printConfig() const {
    std::cout << "\n"
        << "─── Cấu hình hệ thống hiện tại ───────────────────────────────────────────\n"
        << "  • Tổng thời gian mô phỏng (phút) : " << config_.totalSimulationMinutes << "\n"
        << "  • Số lượng phòng khám mở cửa     : " << config_.numberOfRooms << "\n"
        << "  • Thời gian khám min / max (phút): " << config_.minServiceDuration
        << " / " << config_.maxServiceDuration << "\n"
        << "  • Hạt giống sinh ngẫu nhiên      : " << config_.randomSeed << "\n"
        << "  • Chế độ phân loại ưu tiên       : " << (config_.priorityEnabled ? "Bật" : "Tắt") << "\n"
        << "  • Tính năng tăng hạng Aging      : " << (config_.agingEnabled ? "Bật" : "Tắt") << "\n";
    if (config_.agingEnabled) {
        std::cout << "  • Chu kỳ tăng ưu tiên (phút)     : " << config_.agingInterval << "\n";
    }
    std::cout << "  • Ngưỡng chờ tối đa T (bỏ về, ph): " << config_.waitThresholdT << "\n"
        << "──────────────────────────────────────────────────────────────────────────\n";
}

// ─── CHỨC NĂNG 1: Tiếp nhận bệnh nhân & cấp STT ─────────────────────────────

void Menu::handleAdmitPatient() {
    std::cout << "\n─── Tiếp nhận bệnh nhân mới ──────────────────────────────────────────────\n";

    std::string defaultId = "BN";
    if (nextTicketNumber_ < 10) defaultId += "00";
    else if (nextTicketNumber_ < 100) defaultId += "0";
    defaultId += std::to_string(nextTicketNumber_);

    std::string id;
    if (!readLine("  Mã bệnh nhân [" + defaultId + "]: ", id)) return;
    if (id.empty()) id = defaultId;

    std::string fullName;
    while (true) {
        if (!readLine("  Họ và tên: ", fullName)) return;
        if (!fullName.empty()) break;
        std::cout << "  Họ tên không được để trống.\n";
    }

    int age = 30;
    if (!readInt("  Tuổi (0 - 120): ", 0, 120, age)) return;

    std::cout << "  Chọn mức ưu tiên:\n"
              << "    1. Cấp cứu (Ưu tiên cao nhất)\n"
              << "    2. Người cao tuổi (Từ 65 tuổi trở lên)\n"
              << "    3. Trẻ em (Dưới 15 tuổi)\n"
              << "    4. Thông thường\n";
    int prioChoice = 4;
    if (!readInt("  Mức độ (1 - 4) [4]: ", 1, 4, prioChoice)) return;

    std::cout << "  Chuyên khoa khám:\n";
    for (int i = 0; i < kSpecialtyCount; ++i) {
        std::cout << "    " << (i + 1) << ". " << kSpecialties[i];
        if (i % 2 == 1) std::cout << "\n";
        else std::cout << std::string(18 - kSpecialties[i].length(), ' ');
    }
    int specChoice = 1;
    if (!readInt("  Chọn chuyên khoa (1 - 8) [1]: ", 1, 8, specChoice)) return;
    std::string specialty = kSpecialties[specChoice - 1];

    int defaultDuration = (config_.minServiceDuration + config_.maxServiceDuration) / 2;
    int duration = defaultDuration;
    if (!readIntKeep("  Thời gian khám dự kiến (phút)", 1, 120, duration)) return;

    Patient p;
    p.id = id;
    p.fullName = fullName;
    p.age = age;
    p.priority = static_cast<PriorityLevel>(prioChoice);
    p.effectivePriority = prioChoice;
    p.specialty = specialty;
    p.arrivalMinute = 0;
    p.ticketNumber = nextTicketNumber_++;
    p.serviceDuration = duration;
    p.serviceStartMinute = -1;
    p.waitMinutes = 0;
    p.status = PatientStatus::WAITING;

    Room* assignedRoom = liveHospital_->admitPatient(p, config_.priorityEnabled);
    patients_.push_back(p);

    bool inHeap = config_.priorityEnabled && (p.priority != PriorityLevel::NORMAL);

    std::cout << "\n─── TIẾP NHẬN BỆNH NHÂN THÀNH CÔNG ──────────────────────────────────────\n"
              << "  • Bệnh nhân   : " << p.id << " - " << p.fullName << " (" << p.age << " tuổi)\n"
              << "  • Mức ưu tiên : " << StatisticsService::priorityNameVi(prioChoice) << "\n"
              << "  • Số thứ tự   : STT " << p.ticketNumber << "\n";
    if (assignedRoom != nullptr) {
        std::cout << "  • Phân phòng  : " << assignedRoom->getRoomId() << " (" << assignedRoom->getSpecialty() << ") ➔ "
                  << (inHeap ? "Hàng ưu tiên (Min-Heap)" : "Hàng thường (DSLK)") << "\n"
                  << "  • Hàng chờ    : " 
                  << (assignedRoom->getPriorityHeap().size() + assignedRoom->getRegularQueue().size())
                  << " ca đang chờ tại phòng này\n";
    } else {
        std::cout << "  • Cảnh báo    : Không có phòng khám phù hợp\n";
    }
    std::cout << "──────────────────────────────────────────────────────────────────────────\n";
}

// ─── CHỨC NĂNG 2: Gọi bệnh nhân kế tiếp vào khám ───────────────────────────

void Menu::handleCallNextPatient() {
    int roomCount = liveHospital_->getRoomCount();
    if (roomCount == 0) {
        std::cout << "Chưa có phòng khám nào được thiết lập.\n";
        return;
    }

    std::cout << "\n─── Gọi bệnh nhân vào khám ───────────────────────────────────────────────\n"
              << "  1. Gọi bệnh nhân cho tất cả phòng khám\n"
              << "  2. Gọi bệnh nhân cho một phòng cụ thể\n"
              << "  0. Quay lại\n";
    int choice = 0;
    if (!readInt("  Lựa chọn (0 - 2): ", 0, 2, choice) || choice == 0) return;

    auto callForRoom = [this](Room* r) {
        if (r == nullptr) return;
        std::cout << "\n  • Phòng " << r->getRoomId() << " (" << r->getSpecialty() << "):\n";
        if (r->isBusy()) {
            const Patient* cur = r->getCurrentPatient();
            if (cur != nullptr) {
                std::cout << "    (Ca khám trước của " << cur->id << " - " << cur->fullName << " đã hoàn thành)\n";
            }
            r->finishCurrentService();
        }

        Patient* nextP = r->fetchNextPatient(config_.priorityEnabled);
        if (nextP != nullptr) {
            r->startService(nextP, 0);
            std::cout << "    ➔ Mời vào khám: " << nextP->id << " - " << nextP->fullName
                      << " (STT " << nextP->ticketNumber << ", " 
                      << StatisticsService::priorityNameVi(static_cast<int>(nextP->priority))
                      << ", " << nextP->age << " tuổi)\n"
                      << "    ➔ Thời gian khám dự kiến: " << nextP->serviceDuration << " phút\n";
        } else {
            std::cout << "    ➔ Hiện không có bệnh nhân chờ trong hàng đợi.\n";
        }
    };

    if (choice == 1) {
        for (int i = 0; i < roomCount; ++i) {
            callForRoom(liveHospital_->getRoom(i));
        }
    } else {
        std::cout << "\n  Danh sách phòng khám hiện có:\n";
        for (int i = 0; i < roomCount; ++i) {
            Room* r = liveHospital_->getRoom(i);
            int waiting = r->getPriorityHeap().size() + r->getRegularQueue().size();
            std::cout << "    " << (i + 1) << ". " << r->getRoomId()
                      << " (" << r->getSpecialty() << ") - "
                      << (r->isBusy() ? "Đang khám" : "Rảnh")
                      << " [" << waiting << " ca chờ]\n";
        }
        int pick = 1;
        if (!readInt("  Chọn phòng cần gọi (1 - " + std::to_string(roomCount) + "): ", 1, roomCount, pick)) return;
        callForRoom(liveHospital_->getRoom(pick - 1));
    }
}

// ─── CHỨC NĂNG 3: Xem trạng thái phòng khám trực tiếp ──────────────────────

void Menu::handleViewLiveStatus() const {
    int roomCount = liveHospital_->getRoomCount();
    std::cout << "\n"
              << "──────────────────────────────────────────────────────────────────────────────────────────\n"
              << "                        TRẠNG THÁI PHÒNG KHÁM VÀ HÀNG ĐỢI HIỆN TẠI\n"
              << "──────────────────────────────────────────────────────────────────────────────────────────\n  ";
    printUtf8Left(std::cout, "Phòng", 10);
    printUtf8Left(std::cout, "Chuyên khoa", 18);
    printUtf8Left(std::cout, "Trạng thái", 14);
    printUtf8Left(std::cout, "Bệnh nhân đang khám", 28);
    std::cout << "Hàng chờ (Ưu tiên / Thường)\n"
              << "──────────────────────────────────────────────────────────────────────────────────────────\n";

    int totalWaiting = 0;
    for (int i = 0; i < roomCount; ++i) {
        Room* r = liveHospital_->getRoom(i);
        if (r == nullptr) continue;

        std::string statusStr = r->isBusy() ? "Đang khám" : "Rảnh";
        std::string servingStr = "-";
        if (r->isBusy() && r->getCurrentPatient() != nullptr) {
            servingStr = r->getCurrentPatient()->id + " - " + r->getCurrentPatient()->fullName;
            if (servingStr.length() > 25) {
                servingStr = servingStr.substr(0, 22) + "...";
            }
        }

        int heapLen = r->getPriorityHeap().size();
        int regLen = r->getRegularQueue().size();
        totalWaiting += (heapLen + regLen);

        std::string queueStr = std::to_string(heapLen + regLen) + " ca (" +
                               std::to_string(heapLen) + " ưu tiên, " +
                               std::to_string(regLen) + " thường)";

        std::cout << "  ";
        printUtf8Left(std::cout, r->getRoomId(), 10);
        printUtf8Left(std::cout, r->getSpecialty(), 18);
        printUtf8Left(std::cout, statusStr, 14);
        printUtf8Left(std::cout, servingStr, 28);
        std::cout << queueStr << "\n";
    }
    std::cout << "──────────────────────────────────────────────────────────────────────────────────────────\n"
              << "  Tổng cộng: " << totalWaiting << " bệnh nhân đang chờ tại tất cả các phòng khám.\n"
              << "──────────────────────────────────────────────────────────────────────────────────────────\n";
}

// ─── CHỨC NĂNG 4: Mô phỏng ngày làm việc ────────────────────────────────────

void Menu::handleRunSimulation() {
    if (patients_.empty()) {
        std::cout << "\nChưa có danh sách bệnh nhân trong hệ thống.\n";
        std::string confirm;
        if (!readLine("Tự động sinh ngẫu nhiên dữ liệu bệnh nhân cho ngày làm việc? (y/n) [y]: ", confirm)) return;
        if (confirm == "n" || confirm == "N") return;

        int numPatients = 60;
        if (!readIntKeep("Số lượng bệnh nhân đến trong ngày", 1, 100000, numPatients)) return;

        std::string err;
        std::string tempPath = "data/patients_generated_temp.txt";
        if (!FileManager::generatePatients(tempPath, numPatients, config_.randomSeed, err)) {
            std::cout << "Sinh dữ liệu thất bại: " << err << "\n";
            return;
        }
        if (!FileManager::loadPatients(tempPath, patients_, err)) {
            std::cout << "Nạp dữ liệu thất bại: " << err << "\n";
            return;
        }
        std::cout << "Đã sinh ngẫu nhiên và nạp thành công " << patients_.size() << " bệnh nhân.\n";
    }

    if (!checkReadyToRun()) return;

    std::cout << "\nChọn chế độ hiển thị quá trình mô phỏng:\n"
              << "  1. Tóm tắt (Mặc định - Chỉ hiển thị tiến trình và kết quả tổng kết)\n"
              << "  2. Chi tiết sự kiện (Theo dõi thời điểm tiếp nhận, khám, Aging, bỏ về)\n"
              << "  3. Theo dõi từng phút (Hiển thị chi tiết trạng thái phòng khám mỗi phút)\n";
    int modeChoice = 1;
    if (!readInt("Chế độ (1 - 3) [1]: ", 1, 3, modeChoice)) return;

    LogMode mode = LogMode::SUMMARY;
    if (modeChoice == 2) mode = LogMode::EVENTS;
    else if (modeChoice == 3) mode = LogMode::MINUTE_TICK;

    std::cout << "\nBắt đầu mô phỏng ngày làm việc ("
              << config_.totalSimulationMinutes << " phút, "
              << patients_.size() << " bệnh nhân, "
              << config_.numberOfRooms << " phòng khám)...\n\n";

    // Khởi tạo phòng khám cho phiên mô phỏng
    Hospital simHospital;
    std::vector<std::string> specialties;
    for (const auto& pt : patients_) {
        bool exists = false;
        for (const auto& s : specialties) {
            if (s == pt.specialty) { exists = true; break; }
        }
        if (!exists) specialties.push_back(pt.specialty);
    }
    if (specialties.empty()) {
        for (int i = 0; i < kSpecialtyCount; ++i) specialties.push_back(kSpecialties[i]);
    }

    simHospital.createRooms(config_.numberOfRooms, specialties.data(), static_cast<int>(specialties.size()));

    std::vector<Patient> inputEvents = patients_;
    SimulationEngine engine(&simHospital, config_, mode);

    auto start = std::chrono::steady_clock::now();
    engine.runSimulation(inputEvents.data(), static_cast<int>(inputEvents.size()));
    auto stop = std::chrono::steady_clock::now();

    lastResult_.name = "Mô phỏng ngày làm việc";
    lastResult_.config = config_;
    lastResult_.stats = engine.getStatisticsService();
    lastResult_.finalPatients = inputEvents;
    lastResult_.elapsedMs = std::chrono::duration<double, std::milli>(stop - start).count();
    lastResult_.ok = true;
    hasResult_ = true;

    std::cout << "\n[Mô phỏng hoàn tất] Thời gian thực thi: " 
              << std::fixed << std::setprecision(2) << lastResult_.elapsedMs << " ms.\n";

    lastResult_.stats.print(std::cout);
}

// ─── CHỨC NĂNG 5: So sánh các kịch bản ──────────────────────────────────────

void Menu::handleCompareScenarios() {
    if (patients_.empty()) {
        std::cout << "Chưa có dữ liệu bệnh nhân. Vui lòng nạp tệp (mục 7) hoặc tạo dữ liệu (mục 8).\n";
        return;
    }
    if (!checkReadyToRun()) return;

    std::cout << "\n─── So sánh các kịch bản vận hành ────────────────────────────────────────\n"
              << "  1. So sánh 4 kịch bản chính sách (FIFO, Có ưu tiên, Aging, Mở x2 phòng)\n"
              << "  2. So sánh số lượng phòng khám mở cửa (2, 4, 6, 8 phòng...)\n"
              << "  0. Quay lại\n";
    int mode = 1;
    if (!readInt("  Lựa chọn chế độ (0 - 2) [1]: ", 0, 2, mode) || mode == 0) return;

    ScenarioRunner runner(patients_);
    if (mode == 1) {
        std::cout << "Đang chạy mô phỏng so sánh 4 kịch bản chính sách...\n";
        runner.addDefaultScenarios(config_);
    } else {
        handleCompareRoomCounts();
        return;
    }

    int okCount = runner.runAll();
    std::cout << "Hoàn tất " << okCount << "/" << runner.getResults().size() << " kịch bản.\n";
    runner.printComparison(std::cout);
}

void Menu::handleCompareRoomCounts() {
    std::cout << "\nNhập danh sách số phòng khám cần so sánh (phân cách bằng dấu cách) [2 4 6 8]: ";
    std::string line;
    std::getline(std::cin, line);
    line = trimText(line);

    std::vector<int> roomCounts;
    if (line.empty()) {
        roomCounts = {2, 4, 6, 8};
    } else {
        std::stringstream ss(line);
        int val = 0;
        while (ss >> val) {
            if (val > 0) roomCounts.push_back(val);
        }
        if (roomCounts.empty()) roomCounts = {2, 4, 6, 8};
    }

    ScenarioRunner runner(patients_);
    runner.addRoomComparisonScenarios(config_, roomCounts);

    std::cout << "Đang chạy so sánh " << roomCounts.size() << " cấu hình phòng khám...\n";
    int okCount = runner.runAll();

    std::cout << "Hoàn tất " << okCount << "/" << runner.getResults().size() << " kịch bản.\n";
    runner.printComparison(std::cout);
}

// ─── CHỨC NĂNG 6: Xem báo cáo thống kê ──────────────────────────────────────

void Menu::handleShowStatistics() const {
    if (!hasResult_) {
        std::cout << "Chưa có kết quả mô phỏng. Vui lòng thực hiện mô phỏng (mục 4) trước.\n";
        return;
    }
    std::cout << "\n[Báo cáo phiên chạy gần nhất] Thời gian xử lý: " 
              << std::fixed << std::setprecision(2) << lastResult_.elapsedMs << " ms\n";
    lastResult_.stats.print(std::cout);

    // Hiển thị top bệnh nhân chờ lâu nhất nếu có dữ liệu chi tiết
    if (!lastResult_.finalPatients.empty()) {
        int k = 5;
        if (k > static_cast<int>(lastResult_.finalPatients.size())) {
            k = static_cast<int>(lastResult_.finalPatients.size());
        }
        std::vector<Patient> topWait(k);
        SortUtils::topKByWait(lastResult_.finalPatients.data(),
                              static_cast<int>(lastResult_.finalPatients.size()),
                              topWait.data(), k);

        std::cout << "\n  DANH SÁCH BỆNH NHÂN CÓ THỜI GIAN CHỜ LÂU NHẤT (TOP " << k << ")\n  ";
        printUtf8Left(std::cout, "STT", 6);
        printUtf8Left(std::cout, "Mã BN", 10);
        printUtf8Left(std::cout, "Họ và tên", 24);
        printUtf8Left(std::cout, "Mức ưu tiên", 26);
        printUtf8Right(std::cout, "Chờ (phút)", 12);
        std::cout << "   Trạng thái\n"
                  << "  ────────────────────────────────────────────────────────────────────────────────────────\n";

        for (int i = 0; i < k; ++i) {
            const Patient& pt = topWait[i];
            std::string stStr = "Đang chờ";
            if (pt.status == PatientStatus::COMPLETED) stStr = "Đã khám xong";
            else if (pt.status == PatientStatus::LEFT) stStr = "Bỏ về (Quá T)";
            else if (pt.status == PatientStatus::REJECTED) stStr = "Bị từ chối";

            std::cout << "  ";
            printUtf8Left(std::cout, std::to_string(i + 1), 6);
            printUtf8Left(std::cout, pt.id, 10);
            printUtf8Left(std::cout, pt.fullName, 24);
            printUtf8Left(std::cout, StatisticsService::priorityNameVi(static_cast<int>(pt.priority)), 26);
            printUtf8Right(std::cout, std::to_string(pt.waitMinutes), 12);
            std::cout << "   " << stStr << "\n";
        }
        std::cout << "  ────────────────────────────────────────────────────────────────────────────────────────\n";
    }
}

// ─── CÁC CHỨC NĂNG QUẢN LÝ DỮ LIỆU & CẤU HÌNH ──────────────────────────────

void Menu::handleLoadPatients() {
    std::string path;
    if (!readPath("Đường dẫn tệp bệnh nhân", patientsPath_, path)) return;

    std::vector<Patient> loaded;
    std::string error;
    if (!FileManager::loadPatients(path, loaded, error)) {
        std::cout << "Nạp tệp thất bại: " << error << "\n";
        return;
    }

    patients_ = loaded;
    patientsPath_ = path;
    hasResult_ = false;

    // Cập nhật số thứ tự tiếp theo
    int maxTicket = 0;
    for (const auto& pt : patients_) {
        if (pt.ticketNumber > maxTicket) maxTicket = pt.ticketNumber;
    }
    nextTicketNumber_ = maxTicket + 1;

    std::cout << "Đã nạp thành công " << patients_.size() << " hồ sơ bệnh nhân từ: " << path << "\n";
}

void Menu::handleLoadConfig() {
    std::string path;
    if (!readPath("Đường dẫn tệp cấu hình", kDefaultConfigPath, path)) return;

    SimulationConfig loaded = config_;
    std::string error;
    if (!FileManager::loadConfig(path, loaded, error)) {
        std::cout << "Nạp tệp cấu hình thất bại: " << error << "\n";
        return;
    }

    config_ = loaded;
    initLiveHospital();
    hasResult_ = false;
    std::cout << "Đã nạp cấu hình thành công từ: " << path << "\n";
    printConfig();
}

void Menu::handleEditConfig() {
    printConfig();
    std::cout << "\nNhập giá trị mới (nhấn Enter để giữ nguyên giá trị hiện tại):\n";

    SimulationConfig c = config_;
    int seed = static_cast<int>(c.randomSeed);

    if (!readIntKeep("Tổng thời gian mô phỏng trong ngày (phút)", 1, 100000, c.totalSimulationMinutes)) return;
    if (!readIntKeep("Số lượng phòng khám mở cửa", 1, 1000, c.numberOfRooms)) return;
    if (!readIntKeep("Thời gian khám tối thiểu (phút)", 1, 1000, c.minServiceDuration)) return;

    int maxMin = c.maxServiceDuration < c.minServiceDuration
        ? c.minServiceDuration : c.maxServiceDuration;
    c.maxServiceDuration = maxMin;
    if (!readIntKeep("Thời gian khám tối đa (phút)", c.minServiceDuration, 1000, c.maxServiceDuration)) return;

    if (!readIntKeep("Hạt giống ngẫu nhiên (random seed)", 0, 2000000000, seed)) return;
    c.randomSeed = static_cast<unsigned int>(seed);

    if (!readBoolKeep("Bật chế độ hàng đợi ưu tiên", c.priorityEnabled)) return;
    if (!readBoolKeep("Bật tính năng tăng hạng Aging", c.agingEnabled)) return;
    if (c.agingEnabled) {
        if (!readIntKeep("Chu kỳ tăng hạng ưu tiên (Aging interval, phút)", 1, 100000, c.agingInterval)) return;
    }
    if (!readIntKeep("Ngưỡng chờ tối đa T (Bỏ về, phút)", 1, 100000, c.waitThresholdT)) return;

    config_ = c;
    initLiveHospital();
    hasResult_ = false;
    std::cout << "Đã cập nhật cấu hình hệ thống thành công.\n";
}

void Menu::handleGenerateData() {
    std::cout << "\n─── Sinh dữ liệu bệnh nhân ───────────────────────────────────────────────\n"
              << "  1. Tạo bộ dữ liệu mẫu chuẩn (30 bệnh nhân)\n"
              << "  2. Sinh bộ dữ liệu ngẫu nhiên lớn\n"
              << "  0. Quay lại\n";
    int choice = 0;
    if (!readInt("  Lựa chọn (0 - 2): ", 0, 2, choice) || choice == 0) return;

    std::string path;
    std::string error;
    bool ok = false;

    if (choice == 1) {
        if (!readPath("Lưu vào tệp", kDefaultPatientsPath, path)) return;
        ok = FileManager::createSamplePatients(path, error);
    }
    else {
        int count = 100;
        if (!readIntKeep("Số lượng bệnh nhân cần sinh", 1, 1000000, count)) return;
        int seed = static_cast<int>(config_.randomSeed);
        if (!readIntKeep("Hạt giống ngẫu nhiên", 0, 2000000000, seed)) return;
        if (!readPath("Lưu vào tệp", kDefaultLargePath, path)) return;
        ok = FileManager::generatePatients(path, count,
            static_cast<unsigned int>(seed), error);
    }

    if (!ok) {
        std::cout << "Tạo dữ liệu thất bại: " << error << "\n";
        return;
    }
    std::cout << "Đã ghi dữ liệu thành công vào tệp: " << path << "\n";

    std::string answer;
    if (!readLine("Nạp dữ liệu từ tệp này ngay bây giờ? (y/n) [y]: ", answer)) return;
    if (answer != "n" && answer != "N") {
        std::vector<Patient> loaded;
        if (FileManager::loadPatients(path, loaded, error)) {
            patients_ = loaded;
            patientsPath_ = path;
            hasResult_ = false;
            std::cout << "Đã nạp thành công " << patients_.size() << " bệnh nhân vào hệ thống.\n";
        }
        else {
            std::cout << "Nạp tệp thất bại: " << error << "\n";
        }
    }
}

void Menu::handleExportResults() {
    if (!hasResult_) {
        std::cout << "Chưa có kết quả mô phỏng để xuất. Vui lòng chạy mô phỏng (mục 4) trước.\n";
        return;
    }

    std::string path;
    if (!readPath("Đường dẫn tệp xuất kết quả", kDefaultResultPath, path)) return;

    std::string error;
    if (!FileManager::exportSimulationResults(path, lastResult_.finalPatients,
        config_.totalSimulationMinutes, error)) {
        std::cout << "Xuất tệp thất bại: " << error << "\n";
        return;
    }
    std::cout << "Đã xuất kết quả mô phỏng thành công ra: " << path << "\n";
}

bool Menu::checkReadyToRun() const {
    if (patients_.empty()) {
        std::cout << "Chưa có dữ liệu bệnh nhân. Vui lòng nạp tệp (mục 7) hoặc tạo dữ liệu (mục 8).\n";
        return false;
    }
    if (config_.numberOfRooms <= 0 || config_.totalSimulationMinutes <= 0) {
        std::cout << "Cấu hình chưa hợp lệ. Vui lòng kiểm tra lại tại mục 9.\n";
        return false;
    }
    if (config_.agingEnabled &&
        (config_.agingInterval <= 0 || config_.waitThresholdT <= 0)) {
        std::cout << "Tính năng Aging đang bật nhưng thông số chưa hợp lệ. Xem lại mục 9.\n";
        return false;
    }
    return true;
}

// ─── Vòng lặp điều hướng chính ─────────────────────────────────────────────

void Menu::run() {
    while (true) {
        printMenu();
        int choice = 0;
        if (!readInt("Nhập lựa chọn của bạn (0 - 10): ", 0, 10, choice)) break;

        switch (choice) {
        case 1: handleAdmitPatient();       break;
        case 2: handleCallNextPatient();     break;
        case 3: handleViewLiveStatus();      break;
        case 4: handleRunSimulation();       break;
        case 5: handleCompareScenarios();    break;
        case 6: handleShowStatistics();      break;
        case 7: handleLoadPatients();        break;
        case 8: handleGenerateData();        break;
        case 9: handleEditConfig();          break;
        case 10: handleExportResults();      break;
        case 0:
            std::cout << "\nCảm ơn bạn đã sử dụng hệ thống. Chúc một ngày làm việc hiệu quả!\n\n";
            return;
        }
    }
}
