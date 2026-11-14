#pragma once
#include <iosfwd>
// StatisticsService: Thống kê theo mức ưu tiên (1..kMaxPriority, 1 = khẩn cấp nhất).
class StatisticsService {
private:
    int  arrived_[kMaxPriority + 1];
    int  served_[kMaxPriority + 1];
    int  rejected_[kMaxPriority + 1];
    long totalWait_[kMaxPriority + 1];
    int  maxWait_[kMaxPriority + 1];

    static bool validPriority(int p);
public:
    static const int kMaxPriority = 5;
    StatisticsService();
    //Ghi nhận sự kiện
    bool recordArrival(int priority);                // bệnh nhân đến/được tiếp nhận
    bool recordServed(int priority, int waitMinutes);// bệnh nhân được gọi khám
    bool recordRejected(int priority);               // không xếp được phòng
    void reset();
    //Truy vấn theo mức ưu tiên
    int    getArrived(int priority) const;
    int    getServed(int priority) const;
    int    getRejected(int priority) const;
    int    getMaxWait(int priority) const;
    double getAverageWait(int priority) const;    
    //Truy vấn toàn bộ
    int    getTotalArrived() const;
    int    getTotalServed() const;
    int    getTotalRejected() const;
    int    getOverallMaxWait() const;
    double getOverallAverageWait() const;
    // In bảng thống kê
    void print(std::ostream& os) const;
};
