#include "structures/BinaryMinHeap.h"
#include <utility>

// Khởi tạo mảng động với sức chứa ban đầu (mặc định 16)
BinaryMinHeap::BinaryMinHeap(int initialCapacity)
    : heapArray(nullptr),
      currentSize(0),
      capacity(initialCapacity > 0 ? initialCapacity : 16) {
    heapArray = new Patient[capacity];
}

// Copy Constructor: Cấp phát mảng mới và sao chép sâu toàn bộ phần tử
BinaryMinHeap::BinaryMinHeap(const BinaryMinHeap& other)
    : heapArray(new Patient[other.capacity]),
      currentSize(other.currentSize),
      capacity(other.capacity) {
    for (int i = 0; i < currentSize; ++i) {
        heapArray[i] = other.heapArray[i];
    }
}

// Move Constructor: Chuyển quyền sở hữu con trỏ mảng từ heap nguồn sang heap đích (O(1))
BinaryMinHeap::BinaryMinHeap(BinaryMinHeap&& other) noexcept
    : heapArray(other.heapArray),
      currentSize(other.currentSize),
      capacity(other.capacity) {
    other.heapArray = nullptr;
    other.currentSize = 0;
    other.capacity = 0;
}

// Copy Assignment Operator: Giải phóng mảng cũ và sao chép sâu từ đối tượng khác
BinaryMinHeap& BinaryMinHeap::operator=(const BinaryMinHeap& other) {
    if (this == &other) {
        return *this;
    }

    Patient* newArray = new Patient[other.capacity];
    for (int i = 0; i < other.currentSize; ++i) {
        newArray[i] = other.heapArray[i];
    }

    delete[] heapArray;
    heapArray = newArray;
    currentSize = other.currentSize;
    capacity = other.capacity;

    return *this;
}

// Move Assignment Operator: Thu hồi mảng cũ và tiếp quản con trỏ mảng từ đối tượng tạm
BinaryMinHeap& BinaryMinHeap::operator=(BinaryMinHeap&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    delete[] heapArray;

    heapArray = other.heapArray;
    currentSize = other.currentSize;
    capacity = other.capacity;

    other.heapArray = nullptr;
    other.currentSize = 0;
    other.capacity = 0;

    return *this;
}

// Destructor: Giải phóng toàn bộ bộ nhớ cấp phát động tránh memory leak
BinaryMinHeap::~BinaryMinHeap() {
    delete[] heapArray;
}

// Tự động nhân đôi dung lượng mảng khi số phần tử chạm ngưỡng capacity (O(P))
void BinaryMinHeap::ensureCapacity() {
    if (currentSize < capacity) {
        return;
    }

    const int newCapacity = (capacity == 0) ? 16 : capacity * 2;
    Patient* newArray = new Patient[newCapacity];

    for (int i = 0; i < currentSize; ++i) {
        newArray[i] = heapArray[i];
    }

    delete[] heapArray;
    heapArray = newArray;
    capacity = newCapacity;
}

// Hoán đổi vị trí 2 bệnh nhân trong mảng bằng std::move để tối ưu hiệu năng
void BinaryMinHeap::swapPatients(int firstIndex, int secondIndex) {
    Patient temp = std::move(heapArray[firstIndex]);
    heapArray[firstIndex] = std::move(heapArray[secondIndex]);
    heapArray[secondIndex] = std::move(temp);
}

// Vun đống từ dưới lên (Sift-Up): Đẩy nút có độ ưu tiên cao hơn về phía gốc (O(log P))
void BinaryMinHeap::siftUp(int index) {
    while (index > 0) {
        const int parentIndex = (index - 1) / 2;

        // Nếu nút hiện tại không ưu tiên hơn nút cha thì dừng
        if (!heapArray[index].isHigherPriorityThan(heapArray[parentIndex])) {
            break;
        }

        swapPatients(index, parentIndex);
        index = parentIndex;
    }
}

// Vun đống từ trên xuống (Sift-Down): Đẩy nút có độ ưu tiên thấp hơn xuống dưới (O(log P))
void BinaryMinHeap::siftDown(int index) {
    while (true) {
        const int leftChild = 2 * index + 1;
        const int rightChild = 2 * index + 2;
        int highestPriorityIndex = index;

        // So sánh với con trái
        if (leftChild < currentSize &&
            heapArray[leftChild].isHigherPriorityThan(heapArray[highestPriorityIndex])) {
            highestPriorityIndex = leftChild;
        }

        // So sánh với con phải
        if (rightChild < currentSize &&
            heapArray[rightChild].isHigherPriorityThan(heapArray[highestPriorityIndex])) {
            highestPriorityIndex = rightChild;
        }

        // Nếu nút hiện tại đã là ưu tiên nhất trong cụm 3 nút thì dừng
        if (highestPriorityIndex == index) {
            break;
        }

        swapPatients(index, highestPriorityIndex);
        index = highestPriorityIndex;
    }
}

// Chèn bệnh nhân mới vào cuối mảng rồi siftUp lên đúng vị trí (O(log P))
void BinaryMinHeap::insert(const Patient& patient) {
    ensureCapacity();

    heapArray[currentSize] = patient;
    siftUp(currentSize);
    ++currentSize;
}

// Lấy bệnh nhân ưu tiên nhất ở gốc (index 0), đưa phần tử cuối lên thế chỗ rồi siftDown (O(log P))
bool BinaryMinHeap::extractMin(Patient& outPatient) {
    if (isEmpty()) {
        return false;
    }

    outPatient = heapArray[0];
    --currentSize;

    if (currentSize > 0) {
        heapArray[0] = heapArray[currentSize];
        siftDown(0);
    }

    return true;
}

// Xem thông tin bệnh nhân ưu tiên nhất ở gốc mà không xóa khỏi heap (O(1))
bool BinaryMinHeap::peekMin(Patient& outPatient) const {
    if (isEmpty()) {
        return false;
    }

    outPatient = heapArray[0];
    return true;
}

// Tìm tuyến tính và xóa bệnh nhân theo mã ID (dùng khi bệnh nhân bỏ về) (O(P))
bool BinaryMinHeap::removeById(const std::string& id, Patient& removedPatient) {
    int index = -1;

    // 1. Tìm vị trí bệnh nhân trong mảng
    for (int i = 0; i < currentSize; ++i) {
        if (heapArray[i].id == id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return false; // Không tìm thấy
    }

    removedPatient = heapArray[index];
    --currentSize;

    // 2. Đưa phần tử cuối mảng lên lấp chỗ trống và cân bằng lại tại đúng vị trí đó (O(log P))
    if (index < currentSize) {
        heapArray[index] = heapArray[currentSize];
        siftUp(index);
        siftDown(index);
    }

    return true;
}

// Tái lập toàn bộ cấu trúc Min-Heap từ dưới lên (Bottom-up Heapify) sau khi cập nhật Aging (O(P))
void BinaryMinHeap::buildHeap() {
    for (int i = currentSize / 2 - 1; i >= 0; --i) {
        siftDown(i);
    }
}

// Kiểm tra heap có đang rỗng không (O(1))
bool BinaryMinHeap::isEmpty() const {
    return currentSize == 0;
}

// Trả về số lượng bệnh nhân hiện có trong heap (O(1))
int BinaryMinHeap::size() const {
    return currentSize;
}

// Xóa logic toàn bộ phần tử (giữ nguyên vùng nhớ mảng đã cấp phát để tái sử dụng) (O(1))
void BinaryMinHeap::clear() {
    currentSize = 0;
}

// Trả về con trỏ mảng thô để duyệt kiểm tra (ví dụ: cập nhật Aging hàng loạt)
Patient* BinaryMinHeap::getRawArray() {
    return heapArray;
}