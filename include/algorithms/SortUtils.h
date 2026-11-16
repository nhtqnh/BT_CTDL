#pragma once
#include "models/Patient.h"

struct ScenarioResult;

// Các hàm sắp xếp và tìm kiếm dùng cho mảng Patient.
// Dùng mảng thô (Patient[]) không dùng std::sort, std::vector.
namespace SortUtils {

    // So sánh theo arrivalMinute tăng dần (dùng cho qsort/merge).
    bool compareByArrival(const Patient& a, const Patient& b);

    // So sánh theo waitMinutes giảm dần (dùng thống kê).
    bool compareByWaitDesc(const Patient& a, const Patient& b);

    // So sánh theo effectivePriority tăng dần, tie-break arrivalMinute, ticketNumber.
    bool compareByPriority(const Patient& a, const Patient& b);

    // Sắp xếp mảng Patient[n] theo thứ tự đến (arrivalMinute tăng dần).
    // Thuật toán: Merge Sort (ổn định, O(n log n)).
    void sortByArrival(Patient arr[], int n);

    // Sắp xếp mảng Patient[n] theo thứ tự ưu tiên + thời gian đến.
    // Thuật toán: Merge Sort.
    void sortByPriority(Patient arr[], int n);

    // Sắp xếp mảng Patient[n] theo thời gian chờ giảm dần.
    // Thuật toán: Merge Sort.
    void sortByWaitDesc(Patient arr[], int n);

    // Tìm kiếm nhị phân bệnh nhân theo arrivalMinute trong mảng đã sắp xếp.
    // Trả về chỉ số đầu tiên có arrivalMinute == target, hoặc -1.
    int binarySearchByArrival(const Patient arr[], int n, int targetMinute);

    // Lấy top-k bệnh nhân có thời gian chờ lớn nhất từ mảng (không sắp xếp toàn mảng).
    // Kết quả lưu vào out[k]. Dùng partial selection sort.
    void topKByWait(const Patient arr[], int n, Patient out[], int k);

    // Sắp xếp kết quả kịch bản theo thời gian chờ trung bình tăng dần bằng Insertion Sort (O(k^2)).
    void insertionSort(::ScenarioResult results[], int count);

} // namespace SortUtils
