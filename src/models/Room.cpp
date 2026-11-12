
#include "models/Room.h"

Room::Room(std::string id, std::string specialty)
    : roomId(id), specialty(specialty) {
    currentPatient = nullptr;
    remainingServiceTime = 0;
    maxQueueLength = 0;
    totalServedCount = 0;
}

std::string Room::getRoomId() const {
    return roomId;
}

std::string Room::getSpecialty() const {
    return specialty;
}

bool Room::isBusy() const {
    return currentPatient != nullptr;
}


void Room::addPatient(const Patient& p, bool usePriority) {
    if (usePriority && p.priority != PriorityLevel::NORMAL) {
        priorityHeap.insert(p);
    } else {
        regularQueue.enqueue(p);
    }

    int currentQueueLength = regularQueue.size() + priorityHeap.size();

    if (currentQueueLength > maxQueueLength) {
        maxQueueLength = currentQueueLength;
    }
}

Patient* Room::fetchNextPatient(bool usePriority) {
    Patient patient;

    if (usePriority && !priorityHeap.isEmpty()) {
        if (priorityHeap.extractMin(patient)) {
            return new Patient(patient);
        }
    }

    if (!regularQueue.isEmpty()) {
        if (regularQueue.dequeue(patient)) {
            return new Patient(patient);
        }
    }

    return nullptr;
}

void Room::updateServiceTick() {
    if (currentPatient != nullptr && remainingServiceTime > 0) {
        remainingServiceTime--;
    }
}

void Room::startService(Patient* p, int currentMinute) {
    if (p == nullptr || isBusy()) {
        return;
    }

    currentPatient = p;

    currentPatient->status = PatientStatus::IN_SERVICE;
    currentPatient->serviceStartMinute = currentMinute;

    currentPatient->waitMinutes = currentMinute - currentPatient->arrivalMinute;

    if (currentPatient->waitMinutes < 0) {
        currentPatient->waitMinutes = 0;
    }

    remainingServiceTime = currentPatient->serviceDuration;
}

void Room::finishCurrentService() {
    if (currentPatient == nullptr) {
        return;
    }

    currentPatient->status = PatientStatus::COMPLETED;

    totalServedCount++;

    delete currentPatient;
    currentPatient = nullptr;

    remainingServiceTime = 0;
}

LinkedQueue& Room::getRegularQueue() {
    return regularQueue;
}

BinaryMinHeap& Room::getPriorityHeap() {
    return priorityHeap;
}
