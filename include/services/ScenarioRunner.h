#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "models/Patient.h"
#include "models/SimulationConfig.h"
#include "services/StatisticsService.h"

// Kết quả của một kịch bản sau khi chạy
struct ScenarioResult {
    std::string          name;
    SimulationConfig     config;
    StatisticsService    stats;
    std::vector<Patient> finalPatients;
    double               elapsedMs = 0.0;
    bool                 ok = false;
    std::string          error;
};

// Chạy và so sánh nhiều kịch bản mô phỏng trên cùng một tập dữ liệu bệnh nhân
class ScenarioRunner {
public:
    // Khởi tạo với danh sách bệnh nhân ban đầu
    explicit ScenarioRunner(const std::vector<Patient>& patients);

    void addScenario(const std::string& name, const SimulationConfig& config);

    // Thêm sẵn 4 kịch bản mẫu so sánh chính sách (FIFO, Ưu tiên, Aging, Mở rộng phòng)
    void addDefaultScenarios(const SimulationConfig& base);

    void clearScenarios();

    // Tắt/bật log từng phút để tránh ngập màn hình và làm sai lệch thời gian benchmark
    void setVerbose(bool verbose);

    // Chạy toàn bộ các kịch bản đã đăng ký
    int runAll();

    const std::vector<ScenarioResult>& getResults() const;

    // In bảng so sánh kết quả và thời gian benchmark ra màn hình/stream
    void printComparison(std::ostream& os) const;

private:
    std::vector<Patient>        patients_;
    std::vector<std::string>    specialties_;
    std::vector<ScenarioResult> results_;
    bool                        verbose_;

    // Chạy đơn lẻ 1 kịch bản (nơi duy nhất giao tiếp trực tiếp với SimulationEngine)
    bool runSingle(ScenarioResult& result) const;
};
