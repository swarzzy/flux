#pragma once

#include "wctype.h"

#include "StringBuilder.h"

#if defined (PLATFORM_WINDOWS)
const wchar_t PlatformDirectorySeparator = L'\\';
#else
const wchar_t PlatformDirectorySeparator = L'/';
#endif

struct SplitFilePathResult {
    wchar_t* directory;
    wchar_t* filename;
};

SplitFilePathResult SplitFilePath(wchar_t* path) {
    SplitFilePathResult result {};
    auto pathLength = (uptr)wcslen(path);
    uptr splitPos = 0;
    for (uptr i = pathLength - 1; i > 0; i--) {
        if (path[i] == '/' || path[i] == '\\') {
            splitPos = i;
            break;
        }
    }
    if (splitPos == pathLength - 1) {
        result.directory = path;
    } else if (splitPos > 0) {
        path[splitPos] = 0;
        result.directory = path;
        result.filename = path + (splitPos + 1);
    } else {
        result.filename = path + 1;
    }
    return result;
}

void NormalizePath(wchar_t* path) {
#if defined (PLATFORM_WINDOWS)
    while (*path != L'\0') {
        // NOTE: This function might not work as assumed in this case
        *path = towlower(*path);
        if (*path == L'/') {
            *path = L'\\';
        }
        path++;
    }
#endif
}

bool IsDirectorySeparator(wchar_t ch) {
#if defined(PLATFORM_WINDOWS)
    bool result = (ch == L'\\') || (ch == L'/');
#else
    bool result = (ch == L'/');
#endif
    return result;
}

// NOTE: Expects normalized absolute paths
bool GetRelativePath(StringBuilderW* builder, const wchar_t* from, const wchar_t* to) {
#if defined (PLATFORM_WINDOWS)
    // Ensure files are on the same drive
    if (from[0] != to[0]) {
        return false;
    }
#endif
    uptr commonCount = 0;
    bool bothEnds = false;
    while (true) {
        auto fromAt = from[commonCount];
        auto toAt = to[commonCount];
        if ((fromAt == L'\0') || (toAt == L'\0')) {
            if ((fromAt == L'\0') && (toAt == L'\0')) {
                bothEnds = true;
            }
            break;
        }
        if (fromAt != toAt) {
            break;
        }
        commonCount++;
    }
    // Path are equal.
    if (bothEnds) return false;

    u32 fromDirCount = 0;
    auto fromAt = from + commonCount;
    while (*fromAt != L'\0') {
        if (IsDirectorySeparator(*fromAt)) fromDirCount++;
        fromAt++;
    }

    for (u32 i = 0; i < fromDirCount; i++) {
        StringBuilderAppend(builder, L"..");
        StringBuilderAppend(builder, PlatformDirectorySeparator);
    }

    StringBuilderAppend(builder, to + commonCount);

    return true;
}
