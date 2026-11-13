#include "structures/BinaryMinHeap.h"
#include <utility>

BinaryMinHeap::BinaryMinHeap(int initialCapacity)
    : heapArray(nullptr),
      currentSize(0),
      capacity(initialCapacity > 0 ? initialCapacity : 16) {
    heapArray = new Patient[capacity];
}

BinaryMinHeap::BinaryMinHeap(const BinaryMinHeap& other)
    : heapArray(new Patient[other.capacity]),
      currentSize(other.currentSize),
      capacity(other.capacity) {
    for (int i = 0; i < currentSize; ++i) {
        heapArray[i] = other.heapArray[i];
    }
}

BinaryMinHeap::BinaryMinHeap(BinaryMinHeap&& other) noexcept
    : heapArray(other.heapArray),
      currentSize(other.currentSize),
      capacity(other.capacity) {
    other.heapArray = nullptr;
    other.currentSize = 0;
    other.capacity = 0;
}

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

BinaryMinHeap::~BinaryMinHeap() {
    delete[] heapArray;
}

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

void BinaryMinHeap::swapPatients(int firstIndex, int secondIndex) {
    Patient temp = std::move(heapArray[firstIndex]);
    heapArray[firstIndex] = std::move(heapArray[secondIndex]);
    heapArray[secondIndex] = std::move(temp);
}

void BinaryMinHeap::siftUp(int index) {
    while (index > 0) {
        const int parentIndex = (index - 1) / 2;

        if (!heapArray[index].isHigherPriorityThan(heapArray[parentIndex])) {
            break;
        }

        swapPatients(index, parentIndex);
        index = parentIndex;
    }
}

void BinaryMinHeap::siftDown(int index) {
    while (true) {
        const int leftChild = 2 * index + 1;
        const int rightChild = 2 * index + 2;
        int highestPriorityIndex = index;

        if (leftChild < currentSize &&
            heapArray[leftChild].isHigherPriorityThan(heapArray[highestPriorityIndex])) {
            highestPriorityIndex = leftChild;
        }

        if (rightChild < currentSize &&
            heapArray[rightChild].isHigherPriorityThan(heapArray[highestPriorityIndex])) {
            highestPriorityIndex = rightChild;
        }

        if (highestPriorityIndex == index) {
            break;
        }

        swapPatients(index, highestPriorityIndex);
        index = highestPriorityIndex;
    }
}

void BinaryMinHeap::insert(const Patient& patient) {
    ensureCapacity();

    heapArray[currentSize] = patient;
    siftUp(currentSize);
    ++currentSize;
}

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

bool BinaryMinHeap::peekMin(Patient& outPatient) const {
    if (isEmpty()) {
        return false;
    }

    outPatient = heapArray[0];
    return true;
}

bool BinaryMinHeap::removeById(const std::string& id, Patient& removedPatient) {
    int index = -1;

    for (int i = 0; i < currentSize; ++i) {
        if (heapArray[i].id == id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return false;
    }

    removedPatient = heapArray[index];
    --currentSize;

    if (index < currentSize) {
        heapArray[index] = heapArray[currentSize];
        siftUp(index);
        siftDown(index);
    }

    return true;
}

void BinaryMinHeap::buildHeap() {
    for (int i = currentSize / 2 - 1; i >= 0; --i) {
        siftDown(i);
    }
}

bool BinaryMinHeap::isEmpty() const {
    return currentSize == 0;
}

int BinaryMinHeap::size() const {
    return currentSize;
}

void BinaryMinHeap::clear() {
    currentSize = 0;
}

Patient* BinaryMinHeap::getRawArray() {
    return heapArray;
}