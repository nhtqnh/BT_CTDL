#pragma once
#include <string>
#include <vector>
#include "models/Patient.h"
#include "models/SimulationConfig.h"
#include "services/Hospital.h"
#include "services/ScenarioRunner.h"

// Giao diện console quản lý điều hướng và tương tác với người dùng
class Menu {
public:
    Menu();
    ~Menu();

    // Vòng lặp chính của menu, kết thúc khi người dùng chọn thoát (0)
    void run();

private:
    std::vector<Patient> patients_;
    SimulationConfig     config_;
    std::string          patientsPath_;

    Hospital*            liveHospital_;
    int                  nextTicketNumber_;

    bool                 hasResult_;
    ScenarioResult       lastResult_;

    // Khởi tạo phòng khám trực tiếp
    void initLiveHospital();

    // Các chức năng điều phối trực tiếp (Interactive)
    void handleAdmitPatient();
    void handleCallNextPatient();
    void handleViewLiveStatus() const;

    // Các chức năng mô phỏng & so sánh
    void handleRunSimulation();
    void handleCompareScenarios();
    void handleCompareRoomCounts();
    void handleShowStatistics() const;

    // Quản lý dữ liệu & cấu hình
    void handleLoadPatients();
    void handleLoadConfig();
    void handleEditConfig();
    void handleGenerateData();
    void handleExportResults();

    void printMenu() const;
    void printConfig() const;

    // Kiểm tra điều kiện tiên quyết trước khi chạy mô phỏng
    bool checkReadyToRun() const;

    // Nhập liệu an toàn (Chống trôi lệnh, kẹt phím)
    static bool readLine(const std::string& prompt, std::string& out);
    static bool readInt(const std::string& prompt, int minV, int maxV, int& out);
    static bool readIntKeep(const std::string& prompt, int minV, int maxV, int& value);
    static bool readBoolKeep(const std::string& prompt, bool& value);
    static bool readPath(const std::string& prompt, const std::string& defaultPath, std::string& out);
};
