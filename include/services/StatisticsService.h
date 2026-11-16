#pragma once
#include <iosfwd>

// StatisticsService: Thống kê theo mức ưu tiên
class StatisticsService {
private:
    static const int kPriorityCount = 4;

    int  arrived_[kPriorityCount + 1];
    int  served_[kPriorityCount + 1];
    int  left_[kPriorityCount + 1];
    int  rejected_[kPriorityCount + 1];
    long totalWait_[kPriorityCount + 1];
    int  maxWait_[kPriorityCount + 1];
    int  maxQueueLength_;

    static bool validPriority(int p);

public:
    StatisticsService();

    // Tiện ích tên mức ưu tiên
    static const char* priorityName(int priority);
    static const char* priorityNameVi(int priority);

    // Ghi nhận sự kiện
    bool recordArrival(int priority);                // Bệnh nhân đến
    bool recordServed(int priority, int waitMinutes);// Bệnh nhân được gọi khám
    bool recordLeft(int priority);                   // Bệnh nhân bỏ về giữa chừng
    bool recordRejected(int priority);               // Không xếp được phòng / từ chối
    void updateMaxQueueLength(int length);           // Cập nhật độ dài hàng đợi lớn nhất
    void reset();

    // Truy vấn theo mức ưu tiên
    int    getArrived(int priority) const;
    int    getServed(int priority) const;
    int    getLeft(int priority) const;
    int    getRejected(int priority) const;
    int    getMaxWait(int priority) const;
    double getAverageWait(int priority) const;

    // Truy vấn toàn bộ
    int    getTotalArrived() const;
    int    getTotalServed() const;
    int    getTotalLeft() const;
    int    getTotalRejected() const;
    int    getOverallMaxWait() const;
    double getOverallAverageWait() const;
    int    getMaxQueueLength() const;

    // In bảng thống kê
    void print(std::ostream& os) const;
};
