#include "services/ScenarioRunner.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <streambuf>

#include "services/Hospital.h"
#include "services/SimulationEngine.h"

namespace {
    // Ẩn log tạm thời để benchmark chính xác và không ngập màn hình
    class NullBuffer : public std::streambuf {
    protected:
        int overflow(int c) override { return c; }
        std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
    };

    class CoutSilencer {
    public:
        explicit CoutSilencer(bool active) : old_(nullptr) {
            if (active) old_ = std::cout.rdbuf(&null_);
        }
        ~CoutSilencer() {
            if (old_ != nullptr) std::cout.rdbuf(old_);
        }
    private:
        NullBuffer null_;
        std::streambuf* old_;
    };
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
    addScenario("FIFO (khong uu tien)", cfg);

    cfg.priorityEnabled = true;
    addScenario("Uu tien", cfg);

    cfg.agingEnabled = true;
    if (cfg.agingInterval <= 0) cfg.agingInterval = 10;
    if (cfg.waitThresholdT <= 0) cfg.waitThresholdT = 60;
    addScenario("Uu tien + aging", cfg);

    cfg.numberOfRooms = base.numberOfRooms * 2;
    addScenario("Uu tien + aging, x2 phong", cfg);
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
        result.error = "Khong co du lieu benh nhan.";
        return false;
    }
    if (cfg.numberOfRooms <= 0) {
        result.error = "numberOfRooms phai lon hon 0.";
        return false;
    }

    Hospital hospital;
    int created = hospital.createRooms(cfg.numberOfRooms,
        specialties_.data(),
        static_cast<int>(specialties_.size()));
    if (created != cfg.numberOfRooms) {
        result.error = "Khong tao du so phong.";
        return false;
    }

    std::vector<Patient> input = patients_;
    SimulationEngine engine(&hospital, cfg);

    auto start = std::chrono::steady_clock::now();
    {
        CoutSilencer silence(!verbose_);
        engine.runSimulation(input.data(), static_cast<int>(input.size()));
    }
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
    os << std::left << std::setw(28) << "Kich ban"
        << std::setw(7) << "Phong"
        << std::setw(11) << "Tiep nhan"
        << std::setw(9) << "Da kham"
        << std::setw(7) << "Bo ve"
        << std::setw(9) << "Tu choi"
        << std::setw(9) << "TB chung"
        << std::setw(9) << "Max"
        << std::setw(11) << "TB EMERG."
        << std::setw(11) << "TB ELDERLY"
        << std::setw(9) << "TB CHILD"
        << std::setw(10) << "TB NORMAL"
        << std::setw(11) << "Chay (ms)" << "\n";

    os << std::fixed;
    for (size_t i = 0; i < results_.size(); ++i) {
        const ScenarioResult& r = results_[i];

        if (!r.ok) {
            os << std::setw(28) << r.name << "LOI: " << r.error << "\n";
            continue;
        }

        const StatisticsService& s = r.stats;
        os << std::setw(28) << r.name
            << std::setw(7) << r.config.numberOfRooms
            << std::setw(11) << s.getTotalArrived()
            << std::setw(9) << s.getTotalServed()
            << std::setw(7) << s.getTotalLeft()
            << std::setw(9) << s.getTotalRejected()
            << std::setprecision(2)
            << std::setw(9) << s.getOverallAverageWait()
            << std::setw(9) << s.getOverallMaxWait()
            << std::setw(11) << s.getAverageWait(1)
            << std::setw(11) << s.getAverageWait(2)
            << std::setw(9) << s.getAverageWait(3)
            << std::setw(10) << s.getAverageWait(4)
            << std::setprecision(3)
            << std::setw(11) << r.elapsedMs << "\n";
    }
}
