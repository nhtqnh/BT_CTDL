#include "structures/LinkedQueue.h"

// Khởi tạo hàng đợi rỗng
LinkedQueue::LinkedQueue()
    : head(nullptr), tail(nullptr), count(0) {}

// Destructor: Giải phóng toàn bộ node cấp phát động trước khi hủy đối tượng
LinkedQueue::~LinkedQueue() {
    clear();
}

// Hàm hỗ trợ sao chép sâu toàn bộ node từ hàng đợi khác (O(N))
void LinkedQueue::copyFrom(const LinkedQueue& other) {
    QueueNode* current = other.head;
    while (current != nullptr) {
        enqueue(current->data);
        current = current->next;
    }
}

// Copy Constructor: Tạo bản sao độc lập hoàn toàn về vùng nhớ
LinkedQueue::LinkedQueue(const LinkedQueue& other)
    : head(nullptr), tail(nullptr), count(0) {
    copyFrom(other);
}

// Copy Assignment Operator: Xóa dữ liệu cũ và sao chép sâu từ hàng đợi nguồn
LinkedQueue& LinkedQueue::operator=(const LinkedQueue& other) {
    if (this == &other) {
        return *this;
    }

    clear();
    copyFrom(other);
    return *this;
}

// Move Constructor: Chuyển quyền sở hữu danh sách node từ hàng đợi tạm (O(1))
LinkedQueue::LinkedQueue(LinkedQueue&& other) noexcept
    : head(other.head), tail(other.tail), count(other.count) {
    other.head = nullptr;
    other.tail = nullptr;
    other.count = 0;
}

// Move Assignment Operator: Giải phóng node cũ và tiếp quản con trỏ từ đối tượng tạm (O(1))
LinkedQueue& LinkedQueue::operator=(LinkedQueue&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    clear();

    head = other.head;
    tail = other.tail;
    count = other.count;

    other.head = nullptr;
    other.tail = nullptr;
    other.count = 0;

    return *this;
}

// Thêm bệnh nhân vào cuối hàng đợi (O(1))
void LinkedQueue::enqueue(const Patient& patient) {
    QueueNode* newNode = new QueueNode(patient);

    if (isEmpty()) {
        head = newNode;
        tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }

    ++count;
}

// Lấy và xóa bệnh nhân ở đầu hàng đợi (O(1))
bool LinkedQueue::dequeue(Patient& outPatient) {
    if (isEmpty()) {
        return false;
    }

    QueueNode* temp = head;
    outPatient = temp->data;
    head = head->next;

    // Nếu sau khi lấy ra hàng đợi trở nên rỗng, cập nhật tail về nullptr
    if (head == nullptr) {
        tail = nullptr;
    }

    delete temp;
    --count;
    return true;
}

// Xem thông tin bệnh nhân ở đầu hàng đợi nhưng không xóa (O(1))
bool LinkedQueue::peek(Patient& outPatient) const {
    if (isEmpty()) {
        return false;
    }

    outPatient = head->data;
    return true;
}

// Tìm và xóa bệnh nhân theo mã ID - dùng khi bệnh nhân bỏ về hoặc nâng hạng Aging (O(N))
bool LinkedQueue::removeById(const std::string& id, Patient& removedPatient) {
    if (isEmpty()) {
        return false;
    }

    QueueNode* current = head;
    QueueNode* previous = nullptr;

    while (current != nullptr) {
        if (current->data.id == id) {
            removedPatient = current->data;

            // Trường hợp 1: Node cần xóa nằm ở đầu danh sách (head)
            if (previous == nullptr) {
                head = current->next;
                if (head == nullptr) {
                    tail = nullptr; // Danh sách chỉ có 1 phần tử
                }
            }
            // Trường hợp 2: Node cần xóa nằm ở giữa hoặc cuối danh sách
            else {
                previous->next = current->next;
                if (current == tail) {
                    tail = previous; // Cập nhật lại tail nếu xóa phần tử cuối
                }
            }

            delete current;
            --count;
            return true;
        }

        previous = current;
        current = current->next;
    }

    return false; // Không tìm thấy bệnh nhân có mã ID tương ứng
}

// Kiểm tra hàng đợi có rỗng không (O(1))
bool LinkedQueue::isEmpty() const {
    return count == 0;
}

// Trả về số lượng bệnh nhân hiện có trong hàng đợi (O(1))
int LinkedQueue::size() const {
    return count;
}

// Xóa và thu hồi bộ nhớ của toàn bộ node trong danh sách (O(N))
void LinkedQueue::clear() {
    while (head != nullptr) {
        QueueNode* temp = head;
        head = head->next;
        delete temp;
    }

    tail = nullptr;
    count = 0;
}

// Trả về con trỏ node đầu tiên để phục vụ duyệt kiểm tra Aging hoặc bỏ về T (O(1))
QueueNode* LinkedQueue::getHead() const {
    return head;
}