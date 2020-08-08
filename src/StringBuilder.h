#pragma once
#include "Common.h"

template <typename Char>
usize StringLengthZ(const Char* str) {
    usize result = 0;
    while (*str) {
        result++;
        str++;
    }
    result++;
    return result;
}

template <typename Char>
struct StringBuilderT {
    static const uptr GrowFactor = 2;
    Allocator allocator;

    Char* buffer;
    usize bufferCount;
    usize at;
    usize free;
};

typedef StringBuilderT<char> StringBuilder;
typedef StringBuilderT<wchar_t> StringBuilderW;

template <typename Char>
void StringBuilderInit(StringBuilderT<Char>* builder, Allocator allocator, usize size = 64) {
    builder->allocator = allocator;
    builder->buffer = (Char*)allocator.Alloc(sizeof(Char) * size, alignof(Char));
    assert(builder->buffer);
    builder->bufferCount = size;
    builder->at = 0;
    builder->free = size;
};

// TODO: Maybe allocate extra space?
template <typename Char>
void StringBuilderInit(StringBuilderT<Char>* builder, Allocator allocator, const Char* string) {
    builder->allocator = allocator;
    usize size = StringLengthZ(string);
    builder->buffer = (Char*)allocator.Alloc(sizeof(Char) * size, alignof(Char));
    assert(builder->buffer);
    memcpy(builder->buffer, string, sizeof(Char) * size);
    builder->bufferCount = size;
    builder->at = StringLengthZ(string) - 1;
    builder->free = 0;
};

template <typename Char>
void StringBuilderFree(StringBuilderT<Char>* builder) {
    if (builder->buffer) {
        builder->allocator.Dealloc(builder->buffer);
        *builder = {};
    }
}

template <typename Char>
void StringBuilderClear(StringBuilderT<Char>* builder) {
    assert(builder->buffer);
    builder->at = 0;
    builder->free = builder->bufferCount;
    builder->buffer[0] = (Char)0;
}

template <typename Char>
Char* StringBuilderExtractString(StringBuilderT<Char>* builder) {
    auto string = (Char*)builder->allocator.Alloc(sizeof(Char) * (builder->at + 1), alignof(Char));
    memcpy(string, builder->buffer, sizeof(Char) * (builder->at + 1));
    StringBuilderClear(builder);
    return string;
}

template <typename Char>
Char* StringBuilderToString(StringBuilderT<Char>* builder) {
    auto string = builder->buffer;
    *builder = {};
    return string;
}

template <typename Char>
StringBuilderT<Char> StringBuilderClone(StringBuilderT<Char>* builder) {
    StringBuilderT<Char> newBuilder = *builder;
    newBuilder.buffer = newBuilder.allocator.Alloc(sizeof(Char) * builder->bufferCount, alignof(Char), newBuilder.allocator.data);
    assert(newBuilder.buffer);
    memcpy(newBuilder.buffer, builder->buffer, sizeof(Char) * (builder->at + 1));
    return newBuilder;
}

StringBuilderT<char> StringBuilderToASCII(StringBuilderT<wchar_t>* builder) {
    StringBuilderT<char> charBuilder {};
    charBuilder.allocator = builder->allocator;
    charBuilder.bufferCount = builder->bufferCount;
    charBuilder.at = builder->at;
    charBuilder.free = builder->free;

    charBuilder.buffer = (char*)charBuilder.allocator.Alloc(sizeof(char) * builder->bufferCount, alignof(char));
    assert(charBuilder.buffer);
    wcstombs(charBuilder.buffer, builder->buffer, builder->bufferCount * sizeof(char));
    return charBuilder;
}

StringBuilderT<wchar_t> StringBuilderToWide(StringBuilderT<char>* builder) {
    StringBuilderT<wchar_t> wideBuilder {};
    wideBuilder.allocator = builder->allocator;
    wideBuilder.bufferCount = builder->bufferCount;
    wideBuilder.at = builder->at;
    wideBuilder.free = builder->free;

    wideBuilder.buffer = (wchar_t*)wideBuilder.allocator.Alloc(sizeof(wchar_t) * builder->bufferCount, alignof(wchar_t));
    assert(wideBuilder.buffer);
    mbstowcs(wideBuilder.buffer, builder->buffer, builder->bufferCount * sizeof(wchar_t));
    return wideBuilder;
}

template <typename Char>
void StringBuilderGrow(StringBuilderT<Char>* builder, usize newSize) {
    // TODO: Realloc
    assert(builder->bufferCount < newSize);
    auto newMemory = (Char*)builder->allocator.Alloc(sizeof(Char) * newSize, alignof(Char));
    assert(newMemory);
    memcpy(newMemory, builder->buffer, sizeof(Char) * builder->bufferCount);
    builder->allocator.Dealloc(builder->buffer);
    builder->buffer = newMemory;
    builder->bufferCount = newSize;
    builder->free = builder->bufferCount - (builder->at + 1);
}

template <typename Char>
void StringBuilderAppend(StringBuilderT<Char>* builder, Char ch) {
    if (builder->free < 1) {
        StringBuilderGrow(builder, builder->bufferCount * StringBuilderT<Char>::GrowFactor);
    }
    assert(builder->free >= 2);

    builder->buffer[builder->at] = ch;
    builder->at++;
    builder->buffer[builder->at] = (Char)0;
    builder->free = builder->bufferCount - (builder->at + 1);
}

template <typename Char>
void StringBuilderAppend(StringBuilderT<Char>* builder, const Char* string) {
    auto stringCount = StringLengthZ(string);
    if (stringCount > builder->free) {
        auto needs = stringCount - builder->free;
        usize newSize;
        if ((needs + builder->bufferCount) > (builder->bufferCount * StringBuilderT<Char>::GrowFactor)) {
            newSize = needs + 16; // Random margin
        } else {
            newSize = builder->bufferCount * StringBuilderT<Char>::GrowFactor;
        }
        StringBuilderGrow(builder, newSize);
    }
    assert(stringCount <= builder->free);

    memcpy(builder->buffer + builder->at, string, sizeof(Char) * stringCount);
    builder->at += stringCount - 1;
    builder->free = builder->bufferCount - (builder->at + 1);
}
