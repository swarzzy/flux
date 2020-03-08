#include "flux_resource_manager.h"
#include "flux_file_formats.h"

Mesh* ReadMeshFile(void* file, u32 fileSize) {
    auto header = (FluxMeshHeader*)file;
    uptr memorySize = header->dataSize + sizeof(Mesh) * header->entryCount;
    auto memory = PlatformAlloc(memorySize);

    auto entries = (FluxMeshEntry*)((byte*)file + header->entries);
    Mesh* loadedHeaders = (Mesh*)memory;
    void* loadedData= (byte*)memory + sizeof(Mesh) * header->entryCount;
    void* data = (byte*)file + header->data;
    assert((uptr)data % 4 == 0);
    memcpy(loadedData, data, header->dataSize);

    // TODO: Maybe stop copying whole data

    for (u32 i = 0; i < header->entryCount; i++) {
        auto loaded = loadedHeaders + i;
        auto entry = entries + i;

        loaded->base = memory;
        loaded->head = loadedHeaders;
        loaded->next = i == header->entryCount - 1 ? nullptr : loadedHeaders + i + 1;
        loaded->vertexCount = entry->vertexCount;
        loaded->indexCount = entry->indexCount;
        loaded->vertices = (v3*)((byte*)loadedData + (entry->vertices - header->data));
        loaded->normals = (v3*)((byte*)loadedData + (entry->normals - header->data));
        loaded->tangents = (v3*)((byte*)loadedData + (entry->tangents - header->data));
        loaded->indices = (u32*)((byte*)loadedData + (entry->indices - header->data));
        if (entry->uv) {
            loaded->uvs = (v2*)((byte*)loadedData + (entry->uv - header->data));
        } else {
            loaded->uvs = nullptr;
        }
        if (entry->colors) {
            loaded->colors = (v3*)((byte*)loadedData + (entry->colors - header->data));
        } else {
            loaded->colors = nullptr;
        }

        loaded->aabb.min = V3(entry->aabbMin.x, entry->aabbMin.y, entry->aabbMin.z);
        loaded->aabb.max = V3(entry->aabbMax.x, entry->aabbMax.y, entry->aabbMax.z);

        loaded->gpuVertexBufferHandle = 0;
        loaded->gpuIndexBufferHandle = 0;
    }

    return (Mesh*)memory;
}

Mesh* LoadMeshFlux(const wchar_t* filename) {
    Mesh* result = nullptr;
    auto fileSize = PlatformDebugGetFileSize(filename);
    if (fileSize) {
        void* file = PlatformAlloc(fileSize);
        defer { PlatformFree(file); };
        u32 bytesRead = PlatformDebugReadFile(file, fileSize, filename);
        if (bytesRead == fileSize) {
            auto header = (FluxMeshHeader*)file;
            if (header->header.magicValue == FluxFileHeader::MagicValue) {
                if(header->header.type == FluxFileHeader::Mesh) {
                    if (header->version == 1) {
                        result = ReadMeshFile(file, fileSize);
                    } else {
                        // TODO: Logging
                        unreachable();
                    }
                } else {
                    unreachable();
                }
            } else {
                unreachable();
            }
        } else {
            unreachable();
        }
    } else {
        unreachable();
    }
    return result;
}
