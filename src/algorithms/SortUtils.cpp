#include "algorithms/SortUtils.h"
#include "services/ScenarioRunner.h"
#include <cstring>

namespace SortUtils {

// ─── Comparators ────────────────────────────────────────────────────────────

bool compareByArrival(const Patient& a, const Patient& b) {
    return a.arrivalMinute < b.arrivalMinute;
}

bool compareByWaitDesc(const Patient& a, const Patient& b) {
    return a.waitMinutes > b.waitMinutes;
}

bool compareByPriority(const Patient& a, const Patient& b) {
    if (a.effectivePriority != b.effectivePriority)
        return a.effectivePriority < b.effectivePriority;
    if (a.arrivalMinute != b.arrivalMinute)
        return a.arrivalMinute < b.arrivalMinute;
    return a.ticketNumber < b.ticketNumber;
}

// ─── Internal Merge Sort helpers ─────────────────────────────────────────────

static void mergeArrival(Patient arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    Patient* L = new Patient[n1];
    Patient* R = new Patient[n2];

    for (int i = 0; i < n1; ++i) L[i] = arr[left + i];
    for (int j = 0; j < n2; ++j) R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compareByArrival(L[i], R[j]) || L[i].arrivalMinute == R[j].arrivalMinute)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    delete[] L;
    delete[] R;
}

static void mergeSortArrival(Patient arr[], int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortArrival(arr, left, mid);
    mergeSortArrival(arr, mid + 1, right);
    mergeArrival(arr, left, mid, right);
}

static void mergePriority(Patient arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    Patient* L = new Patient[n1];
    Patient* R = new Patient[n2];

    for (int i = 0; i < n1; ++i) L[i] = arr[left + i];
    for (int j = 0; j < n2; ++j) R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compareByPriority(L[i], R[j]) || (!compareByPriority(R[j], L[i])))
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    delete[] L;
    delete[] R;
}

static void mergeSortPriority(Patient arr[], int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortPriority(arr, left, mid);
    mergeSortPriority(arr, mid + 1, right);
    mergePriority(arr, left, mid, right);
}

static void mergeWaitDesc(Patient arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    Patient* L = new Patient[n1];
    Patient* R = new Patient[n2];

    for (int i = 0; i < n1; ++i) L[i] = arr[left + i];
    for (int j = 0; j < n2; ++j) R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compareByWaitDesc(L[i], R[j]) || L[i].waitMinutes == R[j].waitMinutes)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    delete[] L;
    delete[] R;
}

static void mergeSortWaitDesc(Patient arr[], int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortWaitDesc(arr, left, mid);
    mergeSortWaitDesc(arr, mid + 1, right);
    mergeWaitDesc(arr, left, mid, right);
}

// ─── Public API ─────────────────────────────────────────────────────────────

void sortByArrival(Patient arr[], int n) {
    if (n <= 1) return;
    mergeSortArrival(arr, 0, n - 1);
}

void sortByPriority(Patient arr[], int n) {
    if (n <= 1) return;
    mergeSortPriority(arr, 0, n - 1);
}

void sortByWaitDesc(Patient arr[], int n) {
    if (n <= 1) return;
    mergeSortWaitDesc(arr, 0, n - 1);
}

int binarySearchByArrival(const Patient arr[], int n, int targetMinute) {
    int lo = 0, hi = n - 1, result = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (arr[mid].arrivalMinute == targetMinute) {
            result = mid;
            hi = mid - 1; // keep searching left for the first occurrence
        } else if (arr[mid].arrivalMinute < targetMinute) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return result;
}

void topKByWait(const Patient arr[], int n, Patient out[], int k) {
    if (k <= 0 || n <= 0) return;
    if (k > n) k = n;

    // Copy arr into a temporary buffer
    Patient* tmp = new Patient[n];
    for (int i = 0; i < n; ++i) tmp[i] = arr[i];

    // Partial selection sort: pick max k times
    for (int i = 0; i < k; ++i) {
        int maxIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (compareByWaitDesc(tmp[j], tmp[maxIdx])) {
                maxIdx = j;
            }
        }
        // Swap
        Patient temp = tmp[i];
        tmp[i] = tmp[maxIdx];
        tmp[maxIdx] = temp;

        out[i] = tmp[i];
    }

    delete[] tmp;
}

void insertionSort(::ScenarioResult results[], int count) {
    for (int i = 1; i < count; ++i) {
        ::ScenarioResult key = results[i];
        int j = i - 1;
        while (j >= 0 && results[j].stats.getOverallAverageWait() > key.stats.getOverallAverageWait()) {
            results[j + 1] = results[j];
            --j;
        }
        results[j + 1] = key;
    }
}

} // namespace SortUtils
