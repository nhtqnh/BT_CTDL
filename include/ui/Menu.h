#pragma once

#include <string>
#include <vector>

#include "models/Patient.h"
#include "models/SimulationConfig.h"
#include "services/ScenarioRunner.h"

// Giao diện console quản lý điều hướng và tương tác với người dùng
class Menu {
public:
    Menu();

    // Vòng lặp chính của menu, kết thúc khi người dùng chọn thoát (0)
    void run();

private:
    std::vector<Patient> patients_;
    SimulationConfig     config_;
    std::string          patientsPath_;

    bool           hasResult_;
    ScenarioResult lastResult_;

    //Các chức năng điều hướng và xử lý
    void printMenu() const;
    void printConfig() const;
    void handleLoadPatients();
    void handleLoadConfig();
    void handleEditConfig();
    void handleGenerateData();
    void handleRunSimulation();
    void handleShowStatistics() const;
    void handleCompareScenarios();
    void handleExportResults();

    //Kiểm tra điều kiện tiên quyết trước khi chạy mô phỏng
    bool checkReadyToRun() const;

    //Nhập liệu an toàn (Chống trôi lệnh, kẹt phím)
    static bool readLine(const std::string& prompt, std::string& out);
    static bool readInt(const std::string& prompt, int minV, int maxV, int& out);
    static bool readIntKeep(const std::string& prompt, int minV, int maxV, int& value);
    static bool readBoolKeep(const std::string& prompt, bool& value);
    static bool readPath(const std::string& prompt, const std::string& defaultPath, std::string& out);
};
