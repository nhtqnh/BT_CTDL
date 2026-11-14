#pragma once

#include <string>
#include "Patient.h"   
#include "Room.h"     
// ============================================
// MODULE: Hospital Management (Business Logic)
//---------------------------------------------
class Hospital {
private:
    // [Core Data] Quản lý mảng con trỏ Room thủ công
    Room** rooms_;
    int    count_;
    int    capacity_;

    // [Helper] Tự động nhân đôi dung lượng mảng khi đầy
    void grow();

public:
    // [Lifecycle] Khởi tạo & Hủy
    explicit Hospital(int initialCapacity = 8);
    ~Hospital();
    Hospital(const Hospital&) = delete;
    Hospital& operator=(const Hospital&) = delete;

    // [Room Management] Thêm & Tra cứu phòng
    bool  addRoom(Room* room);
    int   getRoomCount() const;
    Room* getRoom(int index) const;
    Room* findRoomById(int roomId) const;
    int   findRoomsBySpecialty(const std::string& specialty, Room** out, int maxOut) const;

    // [Business Logic] Thuật toán chọn phòng & Tiếp nhận bệnh nhân
    Room* chooseRoom(const Patient& patient) const;
    Room* admitPatient(const Patient& patient);

    // [Statistics] Thống kê tải bệnh nhân
    int totalWaiting() const;
};
