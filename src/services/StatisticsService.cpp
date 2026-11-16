#include "services/StatisticsService.h"
#include <iomanip>
#include <ostream>

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

StatisticsService::StatisticsService()
    : maxQueueLength_(0) {
    reset();
}

bool StatisticsService::validPriority(int p) {
    return p >= 1 && p <= kPriorityCount;
}

const char* StatisticsService::priorityName(int priority) {
    switch (priority) {
    case 1: return "EMERGENCY";
    case 2: return "ELDERLY";
    case 3: return "CHILD";
    case 4: return "NORMAL";
    default: return "UNKNOWN";
    }
}

const char* StatisticsService::priorityNameVi(int priority) {
    switch (priority) {
    case 1: return "Cấp cứu (Nhóm 1)";
    case 2: return "Người cao tuổi (Nhóm 2)";
    case 3: return "Trẻ em (Nhóm 3)";
    case 4: return "Thông thường (Nhóm 4)";
    default: return "Không xác định";
    }
}

void StatisticsService::reset() {
    for (int i = 0; i <= kPriorityCount; ++i) {
        arrived_[i] = 0;
        served_[i] = 0;
        left_[i] = 0;
        rejected_[i] = 0;
        totalWait_[i] = 0;
        maxWait_[i] = 0;
    }
    maxQueueLength_ = 0;
}

// Ghi nhận sự kiện

bool StatisticsService::recordArrival(int priority) {
    if (!validPriority(priority)) return false;
    ++arrived_[priority];
    return true;
}

bool StatisticsService::recordServed(int priority, int waitMinutes) {
    if (!validPriority(priority)) return false;
    if (waitMinutes < 0) waitMinutes = 0;
    ++served_[priority];
    totalWait_[priority] += waitMinutes;
    if (waitMinutes > maxWait_[priority]) maxWait_[priority] = waitMinutes;
    return true;
}

bool StatisticsService::recordLeft(int priority) {
    if (!validPriority(priority)) return false;
    ++left_[priority];
    return true;
}

bool StatisticsService::recordRejected(int priority) {
    if (!validPriority(priority)) return false;
    ++rejected_[priority];
    return true;
}

void StatisticsService::updateMaxQueueLength(int length) {
    if (length > maxQueueLength_) {
        maxQueueLength_ = length;
    }
}

//Theo mức ưu tiên

int StatisticsService::getArrived(int p) const {
    return validPriority(p) ? arrived_[p] : 0;
}

int StatisticsService::getServed(int p) const {
    return validPriority(p) ? served_[p] : 0;
}

int StatisticsService::getLeft(int p) const {
    return validPriority(p) ? left_[p] : 0;
}

int StatisticsService::getRejected(int p) const {
    return validPriority(p) ? rejected_[p] : 0;
}

int StatisticsService::getMaxWait(int p) const {
    return validPriority(p) ? maxWait_[p] : 0;
}

double StatisticsService::getAverageWait(int p) const {
    if (!validPriority(p) || served_[p] == 0) return 0.0;
    return static_cast<double>(totalWait_[p]) / served_[p];
}

// Toàn bộ

int StatisticsService::getTotalArrived() const {
    int sum = 0;
    for (int i = 1; i <= kPriorityCount; ++i) sum += arrived_[i];
    return sum;
}

int StatisticsService::getTotalServed() const {
    int sum = 0;
    for (int i = 1; i <= kPriorityCount; ++i) sum += served_[i];
    return sum;
}

int StatisticsService::getTotalLeft() const {
    int sum = 0;
    for (int i = 1; i <= kPriorityCount; ++i) sum += left_[i];
    return sum;
}

int StatisticsService::getTotalRejected() const {
    int sum = 0;
    for (int i = 1; i <= kPriorityCount; ++i) sum += rejected_[i];
    return sum;
}

int StatisticsService::getOverallMaxWait() const {
    int mx = 0;
    for (int i = 1; i <= kPriorityCount; ++i) {
        if (maxWait_[i] > mx) mx = maxWait_[i];
    }
    return mx;
}

double StatisticsService::getOverallAverageWait() const {
    long total = 0;
    int served = 0;
    for (int i = 1; i <= kPriorityCount; ++i) {
        total += totalWait_[i];
        served += served_[i];
    }
    return served == 0 ? 0.0 : static_cast<double>(total) / served;
}

int StatisticsService::getMaxQueueLength() const {
    return maxQueueLength_;
}

// In báo cáo thống kê
void StatisticsService::print(std::ostream& os) const {
    int totalArrived = getTotalArrived();
    int totalServed = getTotalServed();
    int totalLeft = getTotalLeft();
    int totalRejected = getTotalRejected();
    double servedRate = totalArrived > 0 ? (totalServed * 100.0 / totalArrived) : 0.0;
    double leftRate = totalArrived > 0 ? (totalLeft * 100.0 / totalArrived) : 0.0;
    double rejectedRate = totalArrived > 0 ? (totalRejected * 100.0 / totalArrived) : 0.0;

    os << "\n"
       << "────────────────────────────────────────────────────────────────────────────────────────\n"
       << "                      BÁO CÁO THỐNG KÊ HOẠT ĐỘNG KHÁM CHỮA BỆNH\n"
       << "────────────────────────────────────────────────────────────────────────────────────────\n\n"
       << " 1. TỔNG QUAN LƯU LƯỢNG VÀ KẾT QUẢ PHỤC VỤ\n"
       << "    • Tổng bệnh nhân tiếp nhận  : " << std::setw(4) << totalArrived << " ca\n"
       << "    • Đã phục vụ hoàn tất       : " << std::setw(4) << totalServed << " ca (" 
       << std::fixed << std::setprecision(1) << servedRate << "% tổng tiếp nhận)\n"
       << "    • Bệnh nhân bỏ về giữa chừng: " << std::setw(4) << totalLeft << " ca (" 
       << leftRate << "% do chờ quá ngưỡng T)\n"
       << "    • Từ chối tiếp nhận         : " << std::setw(4) << totalRejected << " ca (" 
       << rejectedRate << "% do không có phòng)\n"
       << "    • Độ dài hàng đợi lớn nhất  : " << std::setw(4) << maxQueueLength_ << " ca (tại thời điểm cao điểm)\n\n"
       << " 2. PHÂN TÍCH THỜI GIAN CHỜ THEO NHÓM ĐỘ ƯU TIÊN\n  ";
    printUtf8Left(os, "Nhóm độ ưu tiên", 28);
    printUtf8Right(os, "Tiếp nhận", 12);
    printUtf8Right(os, "Đã khám", 12);
    printUtf8Right(os, "Bỏ về", 10);
    printUtf8Right(os, "Chờ TB (phút)", 16);
    printUtf8Right(os, "Chờ Max (phút)", 16);
    os << "\n  ────────────────────────────────────────────────────────────────────────────────────────\n";

    os << std::fixed << std::setprecision(2);
    for (int p = 1; p <= kPriorityCount; ++p) {
        os << "  ";
        printUtf8Left(os, priorityNameVi(p), 28);
        os << std::right
           << std::setw(12) << arrived_[p]
           << std::setw(12) << served_[p]
           << std::setw(10) << left_[p]
           << std::setw(16) << getAverageWait(p)
           << std::setw(16) << maxWait_[p] << "\n";
    }
    os << "  ────────────────────────────────────────────────────────────────────────────────────────\n"
       << "  ";
    printUtf8Left(os, "Toàn viện tổng hợp", 28);
    os << std::right
       << std::setw(12) << totalArrived
       << std::setw(12) << totalServed
       << std::setw(10) << totalLeft
       << std::setw(16) << getOverallAverageWait()
       << std::setw(16) << getOverallMaxWait() << "\n"
       << "────────────────────────────────────────────────────────────────────────────────────────\n";
}
