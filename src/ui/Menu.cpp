#include "ui/Menu.h"

#include <iostream>
#include <string>

#include "services/FileManager.h"
#include "services/StatisticsService.h"

namespace {
    const char* kDefaultPatientsPath = "data/patients_sample.txt";
    const char* kDefaultLargePath = "data/patients_large.txt";
    const char* kDefaultConfigPath = "data/config.txt";
    const char* kDefaultResultPath = "results/simulation_results.txt";

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
}

Menu::Menu()
    : patientsPath_(kDefaultPatientsPath), hasResult_(false) {
    config_.totalSimulationMinutes = 480;
    config_.numberOfRooms = 6;
    config_.minServiceDuration = 5;
    config_.maxServiceDuration = 20;
    config_.randomSeed = 12345;
    config_.priorityEnabled = true;
    config_.agingEnabled = false;
    config_.agingInterval = 10;
    config_.waitThresholdT = 60;
}

//Nhập liệu

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
        long long v = 0;
        if (parseLong(line, v) && v >= minV && v <= maxV) {
            out = static_cast<int>(v);
            return true;
        }
        std::cout << "  Gia tri khong hop le, nhap so nguyen tu "
            << minV << " den " << maxV << ".\n";
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
        std::cout << "  Gia tri khong hop le, nhap so nguyen tu "
            << minV << " den " << maxV << " (hoac de trong de giu nguyen).\n";
    }
}

bool Menu::readBoolKeep(const std::string& prompt, bool& value) {
    while (true) {
        std::string line;
        if (!readLine(prompt + " (y/n) [" + (value ? "y" : "n") + "]: ", line)) return false;
        if (line.empty()) return true;
        if (line == "y" || line == "Y" || line == "1") { value = true;  return true; }
        if (line == "n" || line == "N" || line == "0") { value = false; return true; }
        std::cout << "  Nhap y hoac n.\n";
    }
}

bool Menu::readPath(const std::string& prompt,
    const std::string& defaultPath, std::string& out) {
    std::string line;
    if (!readLine(prompt + " [" + defaultPath + "]: ", line)) return false;
    out = line.empty() ? defaultPath : line;
    return true;
}

//Hiển thị

void Menu::printMenu() const {
    std::cout << "\n========== QUAN LY & DIEU PHOI BENH NHAN ==========\n"
        << "Benh nhan da nap: " << patients_.size()
        << " | So phong: " << config_.numberOfRooms
        << " | Uu tien: " << (config_.priorityEnabled ? "bat" : "tat")
        << " | Aging: " << (config_.agingEnabled ? "bat" : "tat") << "\n"
        << "---------------------------------------------------\n"
        << " 1. Nap danh sach benh nhan tu file\n"
        << " 2. Nap cau hinh tu file\n"
        << " 3. Xem / sua cau hinh\n"
        << " 4. Tao du lieu benh nhan (mau / lon)\n"
        << " 5. Chay mo phong (cau hinh hien tai)\n"
        << " 6. Xem thong ke lan chay gan nhat\n"
        << " 7. So sanh cac kich ban\n"
        << " 8. Xuat ket qua ra file\n"
        << " 0. Thoat\n";
}

void Menu::printConfig() const {
    std::cout << "\n--- Cau hinh hien tai ---\n"
        << "Tong thoi gian mo phong (phut): " << config_.totalSimulationMinutes << "\n"
        << "So phong                      : " << config_.numberOfRooms << "\n"
        << "Thoi gian kham min/max (phut) : " << config_.minServiceDuration
        << " / " << config_.maxServiceDuration << "\n"
        << "Random seed                   : " << config_.randomSeed << "\n"
        << "Uu tien                       : " << (config_.priorityEnabled ? "bat" : "tat") << "\n"
        << "Aging                         : " << (config_.agingEnabled ? "bat" : "tat") << "\n"
        << "Aging interval (phut)         : " << config_.agingInterval << "\n"
        << "Nguong cho T (bo ve, phut)    : " << config_.waitThresholdT << "\n";
}

//Chức năng

void Menu::handleLoadPatients() {
    std::string path;
    if (!readPath("Duong dan file benh nhan", patientsPath_, path)) return;

    std::vector<Patient> loaded;
    std::string error;
    if (!FileManager::loadPatients(path, loaded, error)) {
        std::cout << "Nap that bai: " << error << "\n";
        return;
    }

    patients_ = loaded;
    patientsPath_ = path;
    hasResult_ = false;
    std::cout << "Da nap " << patients_.size() << " benh nhan tu " << path << "\n";
}

void Menu::handleLoadConfig() {
    std::string path;
    if (!readPath("Duong dan file cau hinh", kDefaultConfigPath, path)) return;

    SimulationConfig loaded = config_;
    std::string error;
    if (!FileManager::loadConfig(path, loaded, error)) {
        std::cout << "Nap that bai: " << error << "\n";
        return;
    }

    config_ = loaded;
    hasResult_ = false;
    std::cout << "Da nap cau hinh tu " << path << "\n";
    printConfig();
}

void Menu::handleEditConfig() {
    printConfig();
    std::cout << "\nNhap gia tri moi (de trong de giu nguyen):\n";

    SimulationConfig c = config_;
    int seed = static_cast<int>(c.randomSeed);

    if (!readIntKeep("Tong thoi gian mo phong (phut)", 1, 100000, c.totalSimulationMinutes)) return;
    if (!readIntKeep("So phong", 1, 1000, c.numberOfRooms)) return;
    if (!readIntKeep("Thoi gian kham toi thieu", 1, 1000, c.minServiceDuration)) return;

    int maxMin = c.maxServiceDuration < c.minServiceDuration
        ? c.minServiceDuration : c.maxServiceDuration;
    c.maxServiceDuration = maxMin;
    if (!readIntKeep("Thoi gian kham toi da", c.minServiceDuration, 1000, c.maxServiceDuration)) return;

    if (!readIntKeep("Random seed", 0, 2000000000, seed)) return;
    c.randomSeed = static_cast<unsigned int>(seed);

    if (!readBoolKeep("Bat uu tien", c.priorityEnabled)) return;
    if (!readBoolKeep("Bat aging", c.agingEnabled)) return;
    if (c.agingEnabled) {
        if (!readIntKeep("Aging interval (phut)", 1, 100000, c.agingInterval)) return;
    }
    if (!readIntKeep("Nguong cho T (phut), cho qua T se bo ve", 1, 100000, c.waitThresholdT)) return;

    config_ = c;
    hasResult_ = false;
    std::cout << "Da cap nhat cau hinh.\n";
}

void Menu::handleGenerateData() {
    std::cout << "\n 1. Tao bo du lieu mau (30 ban ghi)\n"
        << " 2. Sinh bo du lieu lon\n"
        << " 0. Quay lai\n";
    int choice = 0;
    if (!readInt("Chon: ", 0, 2, choice) || choice == 0) return;

    std::string path;
    std::string error;
    bool ok = false;

    if (choice == 1) {
        if (!readPath("Luu vao file", kDefaultPatientsPath, path)) return;
        ok = FileManager::createSamplePatients(path, error);
    }
    else {
        int count = 10000;
        if (!readIntKeep("So benh nhan", 1, 1000000, count)) return;
        int seed = static_cast<int>(config_.randomSeed);
        if (!readIntKeep("Random seed", 0, 2000000000, seed)) return;
        if (!readPath("Luu vao file", kDefaultLargePath, path)) return;
        ok = FileManager::generatePatients(path, count,
            static_cast<unsigned int>(seed), error);
    }

    if (!ok) {
        std::cout << "Tao du lieu that bai: " << error << "\n";
        return;
    }
    std::cout << "Da tao file " << path << "\n";

    std::string answer;
    if (!readLine("Nap file nay ngay bay gio? (y/n): ", answer)) return;
    if (answer == "y" || answer == "Y") {
        std::vector<Patient> loaded;
        if (FileManager::loadPatients(path, loaded, error)) {
            patients_ = loaded;
            patientsPath_ = path;
            hasResult_ = false;
            std::cout << "Da nap " << patients_.size() << " benh nhan.\n";
        }
        else {
            std::cout << "Nap that bai: " << error << "\n";
        }
    }
}

bool Menu::checkReadyToRun() const {
    if (patients_.empty()) {
        std::cout << "Chua co du lieu benh nhan. Hay nap file (muc 1) hoac tao du lieu (muc 4).\n";
        return false;
    }
    if (config_.numberOfRooms <= 0 || config_.totalSimulationMinutes <= 0) {
        std::cout << "Cau hinh chua hop le. Xem muc 3.\n";
        return false;
    }
    if (config_.agingEnabled &&
        (config_.agingInterval <= 0 || config_.waitThresholdT <= 0)) {
        std::cout << "Aging dang bat nhung tham số chua hop le. Xem muc 3.\n";
        return false;
    }
    return true;
}

void Menu::handleRunSimulation() {
    if (!checkReadyToRun()) return;

    if (config_.waitThresholdT <= 0) {
        std::cout << "Luu y: nguong cho T = " << config_.waitThresholdT << ".\n";
    }

    std::string answer;
    if (!readLine("In trang thai tung phut ra man hinh? (y/n) [n]: ", answer)) return;
    bool verbose = (answer == "y" || answer == "Y");

    std::cout << "Dang chay mo phong " << patients_.size() << " benh nhan...\n";

    ScenarioRunner runner(patients_);
    runner.setVerbose(verbose);
    runner.addScenario("Cau hinh hien tai", config_);
    if (runner.runAll() == 0) {
        const ScenarioResult& r = runner.getResults()[0];
        std::cout << "Mo phong that bai: " << r.error << "\n";
        return;
    }

    lastResult_ = runner.getResults()[0];
    hasResult_ = true;

    std::cout << "\nHoan tat trong " << lastResult_.elapsedMs << " ms.\n\n";
    lastResult_.stats.print(std::cout);
}

void Menu::handleShowStatistics() const {
    if (!hasResult_) {
        std::cout << "Chua co ket qua. Hay chay mo phong (muc 5) truoc.\n";
        return;
    }
    std::cout << "\nThong ke lan chay gan nhat (" << lastResult_.elapsedMs << " ms):\n\n";
    lastResult_.stats.print(std::cout);
}

void Menu::handleCompareScenarios() {
    if (!checkReadyToRun()) return;

    std::cout << "Dang chay 4 kich ban so sanh...\n";

    ScenarioRunner runner(patients_);
    runner.addDefaultScenarios(config_);
    int okCount = runner.runAll();

    std::cout << "\nChay thanh cong " << okCount << "/"
        << runner.getResults().size() << " kich ban.\n\n";
    runner.printComparison(std::cout);
}

void Menu::handleExportResults() {
    if (!hasResult_) {
        std::cout << "Chua co ket qua. Hay chay mo phong (muc 5) truoc.\n";
        return;
    }

    bool anyUpdated = false;
    for (size_t i = 0; i < lastResult_.finalPatients.size(); ++i) {
        if (lastResult_.finalPatients[i].status != PatientStatus::WAITING) {
            anyUpdated = true;
            break;
        }
    }
    if (!anyUpdated) {
        std::cout << "Luu y: moi benh nhan van o trang thai WAITING.\n";
        std::string confirm;
        if (!readLine("Van xuat? (y/n): ", confirm)) return;
        if (confirm != "y" && confirm != "Y") return;
    }

    std::string path;
    if (!readPath("Xuat ra file", kDefaultResultPath, path)) return;

    std::string error;
    if (!FileManager::exportSimulationResults(path, lastResult_.finalPatients,
        config_.totalSimulationMinutes, error)) {
        std::cout << "Xuat that bai: " << error << "\n";
        return;
    }
    std::cout << "Da xuat ket qua ra " << path << "\n";
}

//Vòng lặp chính

void Menu::run() {
    while (true) {
        printMenu();
        int choice = 0;
        if (!readInt("Chon chuc nang: ", 0, 8, choice)) break;

        switch (choice) {
        case 1: handleLoadPatients();    break;
        case 2: handleLoadConfig();      break;
        case 3: handleEditConfig();      break;
        case 4: handleGenerateData();    break;
        case 5: handleRunSimulation();   break;
        case 6: handleShowStatistics();  break;
        case 7: handleCompareScenarios(); break;
        case 8: handleExportResults();   break;
        case 0:
            std::cout << "Tam biet!\n";
            return;
        }
    }
}
