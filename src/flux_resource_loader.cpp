#include "flux_platform.h"
#include "flux_math.cpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM

#include "../ext/stb/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION


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

Mesh* ProcessMesh(aiMesh* mesh, const aiScene* scene, const char* name, Mesh* prev, AllocateFn* allocator) {
    uptr verticesCount = mesh->mNumVertices;
    uptr normalsCount = mesh->mNumVertices;
    uptr uvsCount = mesh->mNumVertices;
    uptr colorsCount = mesh->mColors[0] ? verticesCount : 0;
    uptr indicesCount = mesh->mNumFaces * 3;
    uptr tangentsCount = mesh->mNumVertices;

    uptr verticesSize = sizeof(v3) * verticesCount;
    uptr normalsSize = sizeof(v3) * normalsCount;
    uptr uvsSize = sizeof(v2) * uvsCount;
    uptr tangentsSize = sizeof(v3) * tangentsCount;
    uptr colorsSize = sizeof(v3) * colorsCount;
    uptr indicesSize = sizeof(u32) * indicesCount;

    static_assert(sizeof(Mesh) % 4 == 0);
    uptr totalSize = sizeof(Mesh) + verticesSize + normalsSize + uvsSize + tangentsSize + colorsSize + indicesSize;

    byte* memory = (byte*)allocator(totalSize);
    auto loaded = (Mesh*)memory;
    *loaded = {};

    if (!prev) {
        loaded->head = loaded;
    } else {
        prev->next = loaded;
        loaded->head = prev->head;
    }

    auto vertices = (v3*)(memory + sizeof(Mesh));
    assert((uptr)vertices % 4 == 0);
    auto normals = (v3*)((byte*)vertices + verticesSize);
    auto uvs = (v2*)((byte*)normals + normalsSize);
    auto tangents =  (v3*)((byte*)uvs + uvsSize);
    auto colors = (v3*)((byte*)tangents + tangentsSize);
    auto indices = (u32*)((byte*)colors + colorsSize);
    assert(((byte*)indices + indicesSize) == (memory + totalSize));

    memcpy(vertices, mesh->mVertices, verticesSize);
    memcpy(normals, mesh->mNormals, normalsSize);

    assert(mesh->mTextureCoords[0]);
    for (uint i = 0; i < uvsCount; i++) {
        uvs[i] = v2{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
    }

    assert(mesh->mTangents);
    memcpy(tangents, mesh->mTangents, tangentsSize);

    if (colorsCount) {
        for (uint i = 0; i < colorsCount; i++) {
            colors[i] = v3{mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b};
        }
    } else {
        colors = nullptr;
    }

    auto indexIndex = 0;
    for (uint i = 0; i < mesh->mNumFaces; i++) {
        auto face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        indices[indexIndex++] = (u32)face.mIndices[0];
        indices[indexIndex++] = (u32)face.mIndices[1];
        indices[indexIndex++] = (u32)face.mIndices[2];
    }

    auto nameLen = strlen(name);
    if (nameLen > array_count(loaded->name) - 2) {
        nameLen = array_count(loaded->name) - 2;
    }

    memcpy(loaded->name, name, nameLen);
    loaded->name[nameLen + 1] = 0;

    loaded->base = memory;
    loaded->vertexCount = (u32)verticesCount;
    loaded->indexCount = (u32)indicesCount;
    loaded->vertices = (v3*)vertices;
    loaded->normals = (v3*)normals;
    loaded->uvs = (v2*)uvs;
    loaded->tangents = (v3*)tangents;
    loaded->colors = (v3*)colors;
    loaded->indices = (u32*)indices;

    return loaded;
}

Mesh* ProcessAssimpNode(aiNode* node, const aiScene* scene, Mesh* lastLoaded, AllocateFn* allocator) {
    for (uint i = 0; i < node->mNumMeshes; i++) {
        auto mesh = scene->mMeshes[node->mMeshes[i]];
        lastLoaded = ProcessMesh(mesh, scene, node->mName.data, lastLoaded, allocator);
    }

    for (uint i = 0; i < node->mNumChildren; i++) {
        lastLoaded = ProcessAssimpNode(node->mChildren[i], scene, lastLoaded, allocator);
    }
    return lastLoaded;
}

extern "C" GAME_CODE_ENTRY Mesh* __cdecl ResourceLoaderLoadMesh(const char* filename, AllocateFn* allocator) {
    printf("[Resource loader] Loading mesh %s ...", filename);
    auto startTime = GetTimeStamp();

    Assimp::Importer importer;
    auto scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);
    if (!scene || !scene->mRootNode) {
        printf("[Resource loader] Assimp error: %s", importer.GetErrorString());
    }

    auto lastLoaded = ProcessAssimpNode(scene->mRootNode, scene, nullptr, allocator);
    auto meshChain = lastLoaded->head;
    assert(meshChain);

    auto mesh = meshChain;
    while (mesh) {
        mesh->aabb = BBoxAligned::From(mesh);
        mesh = mesh->next;
    }

    auto endTime = GetTimeStamp();
    printf("   Time: %f ms\n", (endTime - startTime) * 1000.0f);

    return meshChain;
}


extern "C" GAME_CODE_ENTRY LoadedImage* __cdecl ResourceLoaderLoadImage(const char* filename, DynamicRange range, b32 flipY, u32 forceBPP, AllocateFn* allocator) {
    printf("[Resource loader] Loading image %s ...", filename);
    auto startTime = GetTimeStamp();

    void* data = nullptr;
    int width;
    int height;
    i32 channels;
    u32 channelSize;

    if (flipY) {
        stbi_set_flip_vertically_on_load(1);
    } else {
        stbi_set_flip_vertically_on_load(0);
    }

    if (range == DynamicRange::LDR) {
        int n;
        data = stbi_load(filename, &width, &height, &n, forceBPP);
        channelSize = sizeof(u8);
        channels = forceBPP ? forceBPP : n;
    } else if (range == DynamicRange::HDR) {
        int n;
        data = stbi_loadf(filename, &width, &height, &n, forceBPP);
        channelSize = sizeof(f32);
        channels = forceBPP ? forceBPP : n;
    } else {
        unreachable();
    }

    LoadedImage* header = nullptr;

    if (data) {
        auto bitmapSize = channelSize * width * height * channels;
        auto size = sizeof(LoadedImage) + bitmapSize;
        auto memory = allocator(size);
        header = (LoadedImage*)memory;
        header->base = memory;
        header->bits = (byte*)memory + sizeof(LoadedImage);
        header->width = width;
        header->height = height;
        header->channels = channels;
        header->range = range;

        auto nameLen = strlen(filename);
        if (nameLen > array_count(header->name) - 2) {
            nameLen = array_count(header->name) - 2;
        }
        memcpy(header->name, filename, nameLen);
        header->name[nameLen + 1] = 0;

        memcpy(header->bits, data, bitmapSize);
        stbi_image_free(data);
    }

    auto endTime = GetTimeStamp();
    printf("   Time: %f ms\n", (endTime - startTime) * 1000.0f);

    return header;
}
