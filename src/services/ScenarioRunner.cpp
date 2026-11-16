#include "services/ScenarioRunner.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>

#include "services/Hospital.h"
#include "services/SimulationEngine.h"

namespace {
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

ScenarioRunner::ScenarioRunner(const std::vector<Patient>& patients)
    : patients_(patients), verbose_(false) {
    // Trích xuất danh sách chuyên khoa theo thứ tự xuất hiện đầu tiên
    for (size_t i = 0; i < patients_.size(); ++i) {
        const std::string& sp = patients_[i].specialty;
        bool exists = false;
        for (size_t j = 0; j < specialties_.size(); ++j) {
            if (specialties_[j] == sp) { exists = true; break; }
        }
        if (!exists) specialties_.push_back(sp);
    }
}

void ScenarioRunner::addScenario(const std::string& name,
    const SimulationConfig& config) {
    ScenarioResult r;
    r.name = name;
    r.config = config;
    results_.push_back(r);
}

void ScenarioRunner::setVerbose(bool verbose) {
    verbose_ = verbose;
}

void ScenarioRunner::addDefaultScenarios(const SimulationConfig& base) {
    SimulationConfig cfg = base;

    if (cfg.waitThresholdT <= 0) cfg.waitThresholdT = 60;

    cfg.priorityEnabled = false;
    cfg.agingEnabled = false;
    addScenario("FIFO (Không ưu tiên)", cfg);

    cfg.priorityEnabled = true;
    addScenario("Có ưu tiên (Min-Heap)", cfg);

    cfg.agingEnabled = true;
    if (cfg.agingInterval <= 0) cfg.agingInterval = 10;
    if (cfg.waitThresholdT <= 0) cfg.waitThresholdT = 60;
    addScenario("Ưu tiên + Tăng hạng Aging", cfg);

    cfg.numberOfRooms = base.numberOfRooms * 2;
    addScenario("Ưu tiên + Aging (Mở x2 phòng)", cfg);
}

void ScenarioRunner::addRoomComparisonScenarios(const SimulationConfig& base, const std::vector<int>& roomCounts) {
    for (int count : roomCounts) {
        if (count <= 0) continue;
        SimulationConfig cfg = base;
        cfg.numberOfRooms = count;
        std::string name = "Mở " + std::to_string(count) + " phòng (" +
                           (cfg.priorityEnabled ? "Có ưu tiên" : "FIFO") +
                           (cfg.agingEnabled ? "+Aging" : "") + ")";
        addScenario(name, cfg);
    }
}

void ScenarioRunner::clearScenarios() {
    results_.clear();
}

const std::vector<ScenarioResult>& ScenarioRunner::getResults() const {
    return results_;
}

bool ScenarioRunner::runSingle(ScenarioResult& result) const {
    const SimulationConfig& cfg = result.config;

    if (patients_.empty()) {
        result.error = "Không có dữ liệu bệnh nhân để mô phỏng.";
        return false;
    }
    if (cfg.numberOfRooms <= 0) {
        result.error = "Số phòng khám (numberOfRooms) phải lớn hơn 0.";
        return false;
    }

    Hospital hospital;
    int created = hospital.createRooms(cfg.numberOfRooms,
        specialties_.data(),
        static_cast<int>(specialties_.size()));
    if (created != cfg.numberOfRooms) {
        result.error = "Không tạo đủ số phòng khám chuyên khoa.";
        return false;
    }

    std::vector<Patient> input = patients_;
    SimulationEngine engine(&hospital, cfg, verbose_ ? LogMode::MINUTE_TICK : LogMode::SUMMARY);

    auto start = std::chrono::steady_clock::now();
    engine.runSimulation(input.data(), static_cast<int>(input.size()));
    auto stop = std::chrono::steady_clock::now();

    result.elapsedMs =
        std::chrono::duration<double, std::milli>(stop - start).count();
    result.stats = engine.getStatisticsService();
    result.finalPatients = input;
    return true;
}

int ScenarioRunner::runAll() {
    int okCount = 0;
    for (size_t i = 0; i < results_.size(); ++i) {
        results_[i].ok = false;
        results_[i].error.clear();
        results_[i].stats.reset();
        results_[i].finalPatients.clear();
        results_[i].elapsedMs = 0.0;

        if (runSingle(results_[i])) {
            results_[i].ok = true;
            ++okCount;
        }
    }
    return okCount;
}

void ScenarioRunner::printComparison(std::ostream& os) const {
    os << "\n"
       << "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n"
       << "                                      BẢNG SO SÁNH HIỆU NĂNG CÁC KỊCH BẢN VẬN HÀNH\n"
       << "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";

    printUtf8Left(os, "Kịch bản vận hành", 32);
    printUtf8Right(os, "Phòng", 8);
    printUtf8Right(os, "Tiếp nhận", 11);
    printUtf8Right(os, "Đã khám", 10);
    printUtf8Right(os, "Bỏ về", 9);
    printUtf8Right(os, "Chờ TB (ph)", 14);
    printUtf8Right(os, "Chờ Max (ph)", 14);
    printUtf8Right(os, "Hàng Max", 11);
    printUtf8Right(os, "TB Cấp cứu", 14);
    printUtf8Right(os, "Chạy (ms)", 12);
    os << "\n──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";

    os << std::fixed;
    int bestAvgWaitIdx = -1;
    double minAvgWait = 1e9;

    int bestEmergIdx = -1;
    double minEmergWait = 1e9;

    for (size_t i = 0; i < results_.size(); ++i) {
        const ScenarioResult& r = results_[i];

        if (!r.ok) {
            printUtf8Left(os, r.name, 32);
            os << "  LỖI: " << r.error << "\n";
            continue;
        }

        const StatisticsService& s = r.stats;
        double overallWait = s.getOverallAverageWait();
        double emergWait = s.getAverageWait(1);

        if (overallWait < minAvgWait && s.getTotalServed() > 0) {
            minAvgWait = overallWait;
            bestAvgWaitIdx = static_cast<int>(i);
        }
        if (emergWait < minEmergWait && s.getServed(1) > 0) {
            minEmergWait = emergWait;
            bestEmergIdx = static_cast<int>(i);
        }

        printUtf8Left(os, r.name, 32);
        os << std::right
           << std::setw(8)  << r.config.numberOfRooms
           << std::setw(11) << s.getTotalArrived()
           << std::setw(10) << s.getTotalServed()
           << std::setw(9)  << s.getTotalLeft()
           << std::setprecision(2)
           << std::setw(14) << overallWait
           << std::setw(14) << s.getOverallMaxWait()
           << std::setw(11) << s.getMaxQueueLength()
           << std::setw(14) << emergWait
           << std::setprecision(2)
           << std::setw(12) << r.elapsedMs << "\n";
    }
    os << "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";

    // Phân tích và làm nổi bật kịch bản tối ưu
    if (bestAvgWaitIdx >= 0) {
        os << " • Kịch bản có thời gian chờ trung bình ngắn nhất : \"" 
           << results_[bestAvgWaitIdx].name << "\" (" 
           << std::fixed << std::setprecision(2) << minAvgWait << " phút)\n";
    }
    if (bestEmergIdx >= 0) {
        os << " • Kịch bản xử lý ca cấp cứu ưu tiên nhanh nhất    : \"" 
           << results_[bestEmergIdx].name << "\" (" 
           << std::fixed << std::setprecision(2) << minEmergWait << " phút)\n";
    }
    os << "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";
}
