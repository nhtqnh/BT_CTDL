#include "services/Hospital.h"

Hospital::Hospital(int initialCapacity)
    : rooms_(nullptr), count_(0),
    capacity_(initialCapacity > 0 ? initialCapacity : 8) {
    rooms_ = new Room * [capacity_];
}

Hospital::~Hospital() {
    for (int i = 0; i < count_; ++i) {
        delete rooms_[i];
    }
    delete[] rooms_;
}

void Hospital::grow() {
    int newCap = capacity_ * 2;
    Room** bigger = new Room * [newCap];
    for (int i = 0; i < count_; ++i) {
        bigger[i] = rooms_[i];
    }
    delete[] rooms_;
    rooms_ = bigger;
    capacity_ = newCap;
}

bool Hospital::addRoom(Room* room) {
    if (room == nullptr) return false;
    if (findRoomById(room->getRoomId()) != nullptr) return false; 
    if (count_ == capacity_) grow();
    rooms_[count_++] = room;
    return true;
}

int Hospital::createRooms(int numberOfRooms,
    const std::string specialties[], int specialtyCount) {
    if (numberOfRooms <= 0 || specialtyCount <= 0) return 0;

    int created = 0;
    for (int i = 0; i < numberOfRooms; ++i) {
        std::string id = "P";
        if (i + 1 < 10) id += "0";
        id += std::to_string(i + 1);

        Room* room = new Room(id, specialties[i % specialtyCount]);
        if (addRoom(room)) {
            ++created;
        }
        else {
            delete room;
        }
    }
    return created;
}

int Hospital::getRoomCount() const {
    return count_;
}

Room* Hospital::getRoom(int index) const {
    if (index < 0 || index >= count_) return nullptr;
    return rooms_[index];
}

Room* Hospital::findRoomById(const std::string& roomId) const {
    for (int i = 0; i < count_; ++i) {
        if (rooms_[i]->getRoomId() == roomId) return rooms_[i];
    }
    return nullptr;
}

Room** Hospital::getRooms() const {
    return rooms_;
}

Room* Hospital::selectBestRoom(const std::string& specialty,
    int avgServiceDuration) const {
    long perPatient = avgServiceDuration > 0 ? avgServiceDuration : 1;
    Room* best = nullptr;
    long bestWait = 0;
    for (int i = 0; i < count_; ++i) {
        Room* r = rooms_[i];
        if (r->getSpecialty() != specialty) continue;
        long estimatedWait = static_cast<long>(getRoomLoad(r)) * perPatient;
        if (best == nullptr || estimatedWait < bestWait) {
            best = r;
            bestWait = estimatedWait;
        }
    }
    return best;
}

int Hospital::findRoomsBySpecialty(const std::string& specialty,
    Room** out, int maxOut) const {
    int found = 0;
    for (int i = 0; i < count_ && found < maxOut; ++i) {
        if (rooms_[i]->getSpecialty() == specialty) {
            out[found++] = rooms_[i];
        }
    }
    return found;
}

int Hospital::getRoomLoad(Room* room) const {
    int load = static_cast<int>(room->getRegularQueue().size())
        + static_cast<int>(room->getPriorityHeap().size());
    if (room->isBusy()) ++load;
    return load;
}

Room* Hospital::chooseRoom(const Patient& patient) const {
    return selectBestRoom(patient.specialty, 1);  
}

Room* Hospital::admitPatient(const Patient& patient, bool usePriority) {
    Room* room = chooseRoom(patient);
    if (room == nullptr) return nullptr;
    room->addPatient(patient, usePriority);
    return room;
}

int Hospital::totalLoad() const {
    int total = 0;
    for (int i = 0; i < count_; ++i) {
        total += getRoomLoad(rooms_[i]);
    }
    return total;
}
