#pragma once
#include "models/Patient.h"
#include "models/SimulationConfig.h"
#include "services/Hospital.h"
#include "services/StatisticsService.h"

enum class LogMode {
    SUMMARY = 0,    // Chế độ 1: Tóm tắt (Mặc định) - Không in log từng phút
    EVENTS = 1,     // Chế độ 2: Chi tiết sự kiện phát sinh (Đến, Khám, Aging, Bỏ về)
    MINUTE_TICK = 2 // Chế độ 3: Theo dõi từng phút - In trạng thái các phòng mỗi phút
};

/**
 * @brief Lớp điều phối và vận hành vòng lặp mô phỏng theo từng phút (Minute Tick Engine).
 */
class SimulationEngine {
private:
    Hospital* hospital;               // Con trỏ tới danh sách phòng khám
    SimulationConfig config;          // Tham số cấu hình mô phỏng
    StatisticsService statsService;   // Service ghi nhận và tổng hợp chỉ số thống kê
    int currentMinute;                // Phút mô phỏng hiện tại trong ngày
    Patient* masterEvents = nullptr;  // Con trỏ tới mảng sự kiện gốc để cập nhật trạng thái
    int masterEventCount = 0;
    LogMode logMode = LogMode::SUMMARY;

    void updateMasterPatient(const std::string& id, PatientStatus status, int waitMin, int startMin);

public:
    /**
     * @brief Khởi tạo engine mô phỏng với bệnh viện và cấu hình tương ứng.
     * @param h Con trỏ quản lý Hospital
     * @param cfg Cấu hình tham số mô phỏng
     * @param mode Chế độ hiển thị mô phỏng (mặc định SUMMARY)
     */
    SimulationEngine(Hospital* h, const SimulationConfig& cfg, LogMode mode = LogMode::SUMMARY);

    void setLogMode(LogMode mode);
    LogMode getLogMode() const { return logMode; }

    /**
     * @brief Vòng lặp thời gian chính chạy từ t = 0 đến totalSimulationMinutes - 1[cite: 2, 4].
     * @param inputEvents Mảng chứa toàn bộ các sự kiện bệnh nhân đến[cite: 4]
     * @param eventCount Tổng số bệnh nhân trong tập sự kiện[cite: 4]
     */
    void runSimulation(Patient inputEvents[], int eventCount);

    /**
     * @brief Nạp và phân tuyến các bệnh nhân đến đúng phút currentMinute vào phòng khám phù hợp[cite: 2, 4].
     * @param arrivals Mảng tất cả sự kiện đến[cite: 4]
     * @param count Số lượng bản ghi trong mảng arrivals[cite: 4]
     */
    void processArrivals(Patient arrivals[], int count);

    /**
     * @brief Kiểm tra và loại bỏ các bệnh nhân đang chờ quá T phút (chuyển trạng thái LEFT)[cite: 2, 4].
     */
    void processAbandonment();

    /**
     * @brief Cập nhật Aging: tăng mức ưu tiên hiệu lực cho bệnh nhân chờ lâu và chuyển từ DSLK sang Heap[cite: 2, 4].
     */
    void processAging();

    /**
     * @brief Cập nhật thời gian khám cho bệnh nhân đang khám và gọi bệnh nhân tiếp theo nếu phòng rảnh[cite: 2, 4].
     */
    void processRoomTick();

    /**
     * @brief Hiển thị trạng thái tóm tắt của tất cả các phòng tại phút hiện tại ra Console[cite: 2, 4].
     */
    void printMinuteStatus() const;

    /**
     * @brief Lấy tham chiếu hằng tới bộ thống kê kết quả mô phỏng[cite: 4].
     * @return const StatisticsService&
     */
    const StatisticsService& getStatisticsService() const;

    /**
     * @brief Lấy thời điểm phút hiện tại của vòng lặp mô phỏng[cite: 4].
     */
    int getCurrentMinute() const { return currentMinute; }
};