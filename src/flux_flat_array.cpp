#include "flux_flat_array.h"

template <typename T>
void FlatArray<T>::Init(usize size, AllocateFn* allocator, DeallocateFn* free, void* allocatorData) {
    assert(!data);
    assert(!capacity);

    this->allocator = allocator;
    this->free = free;
    this->allocatorData = allocatorData;
    this->capacity = size;
    this->count = 0;
    this->initialCount = size;

    if (this->capacity) {
        this->data = (T*)this->allocator(sizeof(T) * this->capacity, alignof(T), this->allocatorData);
        assert(this->data);
    }
}

template <typename T>
T* FlatArray<T>::Push(T value) {
    auto e = this->Push();
    *e = value;
    return e;
}

template <typename T>
T* FlatArray<T>::Push() {
    if (this->count == this->capacity) {
        this->Grow(GrowFactor);
    }
    assert(this->count < this->capacity);

    auto e = this->data + this->count;
    this->count++;
    return e;
}

template <typename T>
T* FlatArray<T>::PushArray(usize count) {
    assert(this->capacity >= this->count);
    auto free = this->capacity - this->count;
    if (free < count) {
        auto needed = count - free;
        auto factor = needed / this->capacity;
        if (factor < GrowFactor) factor = GrowFactor;
        this->Grow(factor);
    }
    free = this->capacity - this->count;
    assert(free >= count);
    auto e = this->data + this->count;
    this->count += count;
    return e;
}

template <typename T>
T* FlatArray<T>::End() {
    T* result = nullptr;
    if (this->count) {
        result = this->data + this->count - 1;
    }
    return result;
}

template <typename T>
void FlatArray<T>::Clear() {
    this->count = 0;
}

template <typename T>
void FlatArray<T>::Resize(usize size) {
    assert(!this->count);
    assert(this->data);
    this->free(this->data, this->allocatorData);
    this->data = (T*)this->allocator(sizeof(T) * size, alignof(T), this->allocatorData);
    assert(this->data);
    this->initialCount = size;
}

template <typename T>
void FlatArray<T>::Resize() {
    this->Resize(this->initialCount);
}

template <typename T>
void FlatArray<T>::Grow(usize factor) {
    // TODO:: Realloc
    assert(this->capacity >= this->count);
    assert(this->capacity < (Usize::Max / factor));
    this->capacity *= factor;
    auto newMemory = (T*)this->allocator(sizeof(T) * this->capacity, alignof(T), this->allocatorData);
    assert(newMemory);
    if (count > 0) {
        memcpy(newMemory, this->data, sizeof(T) * this->count);
    }
    this->free(this->data, this->allocatorData);
    this->data = newMemory;
}
