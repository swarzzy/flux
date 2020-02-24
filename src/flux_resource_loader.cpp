#include "flux_platform.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../ext/tinyobj/tiny_obj_loader.h"

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
static LARGE_INTEGER GlobalPerformanceFrequency = {};

f64 GetTimeStamp() {
    if (GlobalPerformanceFrequency.QuadPart == 0) {
        QueryPerformanceFrequency(&GlobalPerformanceFrequency);
    }
    f64 time = 0.0;
    LARGE_INTEGER currentTime = {};
    if (QueryPerformanceCounter(&currentTime))
    {
        time = (f64)(currentTime.QuadPart) / (f64)GlobalPerformanceFrequency.QuadPart;
    }
    return time;
}
#else
f64 GetTimeStamp() { return 0; }
#endif



struct v3 {
    f32 data[3];
};

struct v2 {
    f32 data[2];
};

extern "C" GAME_CODE_ENTRY void ResourceLoaderLoadMesh(const char* filename, AllocateFn* allocator, LoadedMesh* mesh) {
    assert(!mesh->base);
    mesh->base = nullptr;

    printf("[Resource loader] Loading mesh %s ...", filename);
    auto startTime = GetTimeStamp();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings;
    std::string errors;

    bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, &errors, filename);

    if (!errors.empty()) {
        printf("%s", errors.c_str());
    }

    if (result) {
        if (shapes.size() == 1) {
            tinyobj::index_t* _indices = shapes[0].mesh.indices.data();

            f32* _positions = attrib.vertices.data();
            v3* _normals = (v3*)attrib.normals.data();
            v2* _uvs = (v2*)attrib.texcoords.data();

            uptr positionsSize = sizeof(f32) * attrib.vertices.size();
            uptr normalsSize = positionsSize;
            uptr uvsSize = positionsSize / 3 * 2;
            uptr indicesSize = sizeof(u32) * shapes[0].mesh.indices.size();
            uptr tangentsSize = positionsSize;

            uptr totalSize = positionsSize + normalsSize + uvsSize + indicesSize + tangentsSize;

            byte* memory = (byte*)allocator(totalSize);
            memset(memory, 0, totalSize);

            f32* vertices = (f32*)memory;
            v3* normals = (v3*)((byte*)vertices + positionsSize);
            v2* uvs = (v2*)((byte*)normals + normalsSize);
            u32* indices = (u32*)((byte*)uvs + uvsSize);
            void* tangents = (byte*)indices + indicesSize;
            assert((uptr)((byte*)tangents + tangentsSize) == ((uptr)memory + totalSize));

            memcpy(vertices, _positions, positionsSize);

            for (u32x i = 0; i < shapes[0].mesh.indices.size(); i++) {
                normals[_indices[i].vertex_index] = _normals[_indices[i].normal_index];
            }

            for (u32x i = 0; i < shapes[0].mesh.indices.size(); i++) {
                uvs[_indices[i].vertex_index] = _uvs[_indices[i].texcoord_index];
            }

            for (u32x i = 0; i < shapes[0].mesh.indices.size(); i++) {
                indices[i] = _indices[i].vertex_index;
            }

            mesh->base = memory;
            mesh->vertexCount = (u32)attrib.vertices.size() / 3;
            mesh->indexCount = (u32)shapes[0].mesh.indices.size();
            mesh->vertices = (f32*)vertices;
            mesh->normals = (f32*)normals;
            mesh->uvs = (f32*)uvs;
            mesh->tangents = (f32*)tangents;
            mesh->indices = (u32*)indices;

            auto endTime = GetTimeStamp();
            printf("    Time: %f ms\n", (endTime - startTime) * 1000.0f);
        } else {
            printf("[Resource loader]: Multiple objects are not supported\n");
        }
    }
}
