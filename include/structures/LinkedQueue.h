#pragma once
#define LINKED_QUEUE_H

#include <string>
#include "models/Patient.h"

// Node của danh sách liên kết đơn dùng để cài đặt hàng đợi bệnh nhân thường.
struct QueueNode {
    Patient data;
    QueueNode* next;

    explicit QueueNode(const Patient& patient)
        : data(patient), next(nullptr) {}
};

// Hàng đợi FIFO cho bệnh nhân không thuộc diện ưu tiên.
// Cài đặt bằng danh sách liên kết đơn, không sử dụng std::queue hoặc std::list.
class LinkedQueue {
public:
    LinkedQueue();
    ~LinkedQueue();

    // Sao chép sâu để mỗi hàng đợi tự quản lý vùng nhớ của mình.
    LinkedQueue(const LinkedQueue& other);
    LinkedQueue& operator=(const LinkedQueue& other);

    // Chuyển quyền sở hữu các node từ hàng đợi nguồn.
    LinkedQueue(LinkedQueue&& other) noexcept;
    LinkedQueue& operator=(LinkedQueue&& other) noexcept;

    // Thêm bệnh nhân vào cuối hàng đợi. Độ phức tạp: O(1).
    void enqueue(const Patient& patient);

    // Lấy và xóa bệnh nhân ở đầu hàng đợi.
    // Trả về false nếu hàng đợi rỗng. Độ phức tạp: O(1).
    bool dequeue(Patient& outPatient);

    // Sao chép thông tin bệnh nhân ở đầu hàng đợi nhưng không xóa.
    // Trả về false nếu hàng đợi rỗng. Độ phức tạp: O(1).
    bool peek(Patient& outPatient) const;

    // Tìm và xóa bệnh nhân theo mã ID.
    // Trả về false nếu không tìm thấy. Độ phức tạp: O(N).
    bool removeById(const std::string& id, Patient& removedPatient);

    bool isEmpty() const;
    int size() const;

    // Xóa toàn bộ node; sau lời gọi, hàng đợi rỗng.
    void clear();

    // Trả về node đầu để các module nội bộ có thể duyệt hàng đợi.
    // Con trỏ trả về không chuyển quyền sở hữu node cho bên gọi.
    QueueNode* getHead() const;

private:
    QueueNode* head;
    QueueNode* tail;
    int count;

    // Sao chép toàn bộ node từ một hàng đợi khác (sao chép sâu).
    void copyFrom(const LinkedQueue& other);
};

#endif // LINKED_QUEUE_H
