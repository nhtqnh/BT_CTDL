#ifndef ROOM_H
#define ROOM_H

#include <string>
#include "models/Patient.h"
#include "structures/LinkedQueue.h"
#include "structures/BinaryMinHeap.h"

class Room {
private:
    std::string roomId;
    std::string specialty;

    LinkedQueue regularQueue;
    BinaryMinHeap priorityHeap;

    Patient* currentPatient = nullptr;

    int remainingServiceTime = 0;
    int maxQueueLength = 0;
    int totalServedCount = 0;

public:

    Room(std::string id, std::string specialty);

    std::string getRoomId() const;
    std::string getSpecialty() const;

    bool isBusy() const;

    void addPatient(const Patient& p, bool usePriority);

    Patient* fetchNextPatient(bool usePriority);

    void updateServiceTick();

    void startService(Patient* p, int currentMinute);

    void finishCurrentService();

    LinkedQueue& getRegularQueue();
    BinaryMinHeap& getPriorityHeap();
};

#endif