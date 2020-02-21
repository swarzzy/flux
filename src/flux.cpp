#include "flux_platform.h"
#include "flux_intrinsics.cpp"
#include "flux_math.cpp"

void* PlatformCalloc(uptr num, uptr size) { void* ptr = PlatformAlloc(num * size); memset(ptr, 0, num * size); return ptr; }

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#define TINYOBJ_MALLOC PlatformAlloc
#define TINYOBJ_REALLOC PlatformRealloc
#define TINYOBJ_CALLOC PlatformCalloc
#define TINYOBJ_FREE PlatformFree

#include "../ext/tinyobj/tinyobj_loader_c.h"

void FluxInit() {}
void FluxReload() {}
void FluxUpdate() {}
void FluxRender() {}
