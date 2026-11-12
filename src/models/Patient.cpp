#include "models/Patient.h"

 bool Patient::isHigherPriorityThan(const Patient& other) const {
    if (effectivePriority != other.effectivePriority) {
        return effectivePriority < other.effectivePriority;
    }

    if (arrivalMinute != other.arrivalMinute) {
        return arrivalMinute < other.arrivalMinute;
    }

    return ticketNumber < other.ticketNumber;
}