#include "services/StatisticsService.h"

#include <iomanip>
#include <ostream>

StatisticsService::StatisticsService() {
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

void StatisticsService::reset() {
    for (int i = 0; i <= kPriorityCount; ++i) {
        arrived_[i] = 0;
        served_[i] = 0;
        left_[i] = 0;
        rejected_[i] = 0;
        totalWait_[i] = 0;
        maxWait_[i] = 0;
    }
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

//In bảng

void StatisticsService::print(std::ostream& os) const {
    os << std::left
        << std::setw(12) << "Uu tien"
        << std::setw(11) << "Tiep nhan"
        << std::setw(10) << "Da kham"
        << std::setw(8) << "Bo ve"
        << std::setw(10) << "Tu choi"
        << std::setw(14) << "Cho TB (ph)"
        << std::setw(14) << "Cho max (ph)" << "\n";

    os << std::fixed << std::setprecision(2);
    for (int p = 1; p <= kPriorityCount; ++p) {
        os << std::setw(12) << priorityName(p)
            << std::setw(11) << arrived_[p]
            << std::setw(10) << served_[p]
            << std::setw(8) << left_[p]
            << std::setw(10) << rejected_[p]
            << std::setw(14) << getAverageWait(p)
            << std::setw(14) << maxWait_[p] << "\n";
    }
    os << std::setw(12) << "Tong"
        << std::setw(11) << getTotalArrived()
        << std::setw(10) << getTotalServed()
        << std::setw(8) << getTotalLeft()
        << std::setw(10) << getTotalRejected()
        << std::setw(14) << getOverallAverageWait()
        << std::setw(14) << getOverallMaxWait() << "\n";
}
