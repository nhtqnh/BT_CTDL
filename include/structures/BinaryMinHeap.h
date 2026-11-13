#pragma once
#define BINARY_MIN_HEAP_H

#include <string>
#include "models/Patient.h"

// Hàng đợi ưu tiên cài đặt bằng Binary Min-Heap trên mảng động.
// Patient::isHigherPriorityThan() xác định thứ tự ưu tiên:
// (effectivePriority, arrivalMinute, ticketNumber) tăng dần.
// Giá trị khóa nhỏ hơn nằm gần gốc heap hơn.
// Không sử dụng std::vector hoặc std::priority_queue.
class BinaryMinHeap {
public:
    explicit BinaryMinHeap(int initialCapacity = 16);
    ~BinaryMinHeap();

    // Sao chép sâu để heap đích sở hữu mảng riêng.
    BinaryMinHeap(const BinaryMinHeap& other);
    BinaryMinHeap& operator=(const BinaryMinHeap& other);

    // Chuyển quyền sở hữu mảng từ heap nguồn.
    BinaryMinHeap(BinaryMinHeap&& other) noexcept;
    BinaryMinHeap& operator=(BinaryMinHeap&& other) noexcept;

    // Thêm bệnh nhân và điều chỉnh heap lên trên. Độ phức tạp: O(log P).
    void insert(const Patient& patient);

    // Lấy và xóa bệnh nhân có mức ưu tiên cao nhất ở gốc heap.
    // Trả về false nếu heap rỗng. Độ phức tạp: O(log P).
    bool extractMin(Patient& outPatient);

    // Xem bệnh nhân ở gốc heap nhưng không xóa.
    // Trả về false nếu heap rỗng. Độ phức tạp: O(1).
    bool peekMin(Patient& outPatient) const;

    // Tìm và xóa bệnh nhân theo ID. Độ phức tạp: O(P) do phải tìm tuyến tính.
    bool removeById(const std::string& id, Patient& removedPatient);

    // Xây dựng lại min-heap bằng Bottom-up Heapify. Độ phức tạp: O(P).
    void buildHeap();

    bool isEmpty() const;
    int size() const;

    // Xóa các phần tử logic nhưng giữ lại dung lượng mảng đã cấp phát.
    void clear();

    // Trả về con trỏ mảng nội bộ để duyệt/kiểm tra heap.
    // Không được delete, thay thế hoặc lưu con trỏ này sau khi heap bị thay đổi.
    Patient* getRawArray();

private:
    Patient* heapArray;
    int currentSize;
    int capacity;

    // Mở rộng mảng động khi số phần tử đạt sức chứa.
    void ensureCapacity();

    // Đưa phần tử tại index lên đúng vị trí.
    void siftUp(int index);

    // Đưa phần tử tại index xuống đúng vị trí.
    void siftDown(int index);

    // Hoán đổi hai bệnh nhân trong mảng heap.
    void swapPatients(int firstIndex, int secondIndex);
};

#endif // BINARY_MIN_HEAP_H
