#pragma once

#include "../flux-platform/src/Common.h"

template <typename T>
struct FlatArray {
    static const u32 GrowFactor = 2;

    AllocateFn* allocator;
    DeallocateFn* free;
    void* allocatorData;

    T* data;
    usize capacity;
    usize count;
    usize initialCount;

    void Init(usize size, AllocateFn allocator = PlatformAlloc, DeallocateFn* free = PlatformFree, void* allocatorData = nullptr);
    T* Push(T value);
    T* Push();
    T* PushArray(usize count);
    T* End();
    void Clear();
    void Resize(usize size);
    void Resize();
    void Grow(usize factor);
};
