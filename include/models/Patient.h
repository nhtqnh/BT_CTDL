
#ifndef PATIENT_H
#define PATIENT_H
#include <string>
enum class PriorityLevel {
    EMERGENCY = 1,
    ELDERLY = 2,
    CHILD = 3,
    NORMAL = 4
};

enum class PatientStatus {
    WAITING,
    IN_SERVICE,
    COMPLETED,
    LEFT,
    REJECTED
};

struct Patient {
    std::string id;
    std::string fullName;
    int age = 0;

    PriorityLevel priority = PriorityLevel::NORMAL;
    int effectivePriority = 4;

    std::string specialty;

    int arrivalMinute = 0;
    int ticketNumber = 0;
    int serviceDuration = 0;
    int serviceStartMinute = -1;
    int waitMinutes = 0;

    PatientStatus status = PatientStatus::WAITING;

    bool isHigherPriorityThan(const Patient& other) const;
};

#endif
