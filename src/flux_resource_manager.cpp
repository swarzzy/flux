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

// TODO: Asset names encoding
struct LoadMeshWorkData {
    const wchar_t* filename;
    enum struct MeshType {AAB, Flux} meshType;
    MeshSlot* result;
};

void LoadMeshWork(void* _data, u32 threadIndex) {
    PlatformSleep(1000);
    auto data = (LoadMeshWorkData*)_data;
    wprintf(L"[Info] Thread %d: Loading mesh %s\n", (int)threadIndex, data->filename);
    Mesh* mesh = nullptr;
    switch (data->meshType)
    {
    case LoadMeshWorkData::MeshType::AAB: {
        mesh = LoadMeshAABAsync(data->filename);
    } break;
    case LoadMeshWorkData::MeshType::Flux: {
        mesh = LoadMeshFlux(data->filename);
    } break;
    invalid_default();
    }

    WriteFence();
    // TODO: Fence
    // TODO: volatile?
    data->result->mesh = mesh;
    data->result->state = MeshSlot::JustLoaded;
    // TODO: Use arenas for this allocations
    PlatformFree(data);
}

void LoadMesh(AssetManager* manager, EntityMesh id) {
    switch (id) {
    case EntityMesh::Backpack: {
        auto slot = manager->meshes + (u32)EntityMesh::Backpack;
        auto spec = (LoadMeshWorkData*)PlatformAlloc(sizeof(LoadMeshWorkData));
        *spec = {};
        spec->filename = L"../res/meshes/backpack_low.mesh";
        spec->meshType = LoadMeshWorkData::MeshType::Flux;
        spec->result = slot;
        slot->state = MeshSlot::Loading;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadMeshWork);
    } break;
    case EntityMesh::Sphere: {
        auto slot = manager->meshes + (u32)EntityMesh::Sphere;
        auto spec = (LoadMeshWorkData*)PlatformAlloc(sizeof(LoadMeshWorkData));
        *spec = {};
        spec->filename = L"../res/meshes/sphere.aab";
        spec->meshType = LoadMeshWorkData::MeshType::AAB;
        spec->result = slot;
        slot->state = MeshSlot::Loading;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadMeshWork);
    } break;
    case EntityMesh::Plate: {

        auto slot = manager->meshes + (u32)EntityMesh::Plate;
        auto spec = (LoadMeshWorkData*)PlatformAlloc(sizeof(LoadMeshWorkData));
        *spec = {};
        spec->filename = L"../res/meshes/plate.aab";
        spec->meshType = LoadMeshWorkData::MeshType::AAB;
        spec->result = slot;
        slot->state = MeshSlot::Loading;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadMeshWork);
    } break;
    invalid_default();
    }
}

struct PbrMaterialSpec {
    const char* albedo;
    const char* roughness;
    const char* metallic;
    const char* normals;
    MaterialSlot* result;
};

struct LegacyMaterialSpec {
    const char* diffuse;
    const char* specular;
    MaterialSlot* result;
};

void LoadPbrMaterialWork(void* materialSpec, u32 threadIndex) {
    PlatformSleep(1000);
    auto spec = (PbrMaterialSpec*)materialSpec;
    printf("[Info] Thread %d: Loading material %s\n", (int)threadIndex, spec->albedo);
    auto material = LoadMaterialPBRMetallicAsync(spec->albedo, spec->roughness, spec->metallic, spec->normals);
    // TODO: Fence
    // TODO: volatile?
    spec->result->material = material;
    spec->result->state = MaterialSlot::JustLoaded;
    // TODO: Use arenas for this allocations
    PlatformFree(spec);
}

void LoadLegacyMaterialWork(void* materialSpec, u32 threadIndex) {
    PlatformSleep(1000);
    auto spec = (LegacyMaterialSpec*)materialSpec;
    printf("[Info] Thread %d: Loading material %s\n", (int)threadIndex, spec->diffuse);
    auto material = LoadMaterialLegacyAsync(spec->diffuse, spec->specular);
    // TODO: Fence
    // TODO: volatile?
    spec->result->material = material;
    spec->result->state = MaterialSlot::JustLoaded;
    PlatformFree(spec);
}

void LoadMaterial(AssetManager* manager, EntityMaterial id) {
    switch (id) {
    case EntityMaterial::OldMetal: {
        auto slot = manager->materials + (u32)EntityMaterial::OldMetal;

        // TODO: Work with memory??? Use arenas
        auto spec = (PbrMaterialSpec*)PlatformAlloc(sizeof(PbrMaterialSpec));
        *spec = {};
        spec->albedo = "../res/materials/oldmetal/greasy-metal-pan1-albedo.png";
        spec->roughness = "../res/materials/oldmetal/greasy-metal-pan1-roughness.png";
        spec->metallic = "../res/materials/oldmetal/greasy-metal-pan1-metal.png";
        spec->normals = "../res/materials/oldmetal/greasy-metal-pan1-normal.png";
        spec->result = slot;
        slot->state = MaterialSlot::Loading;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadPbrMaterialWork);
    } break;
    case EntityMaterial::Backpack: {
        auto slot = manager->materials + (u32)EntityMaterial::Backpack;

        auto spec = (PbrMaterialSpec*)PlatformAlloc(sizeof(PbrMaterialSpec));
        *spec = {};
        spec->albedo = "../res/materials/backpack/albedo.png";
        spec->roughness = "../res/materials/backpack/rough.png";
        spec->metallic = "../res/materials/backpack/metallic.png";
        spec->normals = "../res/materials/backpack/normal.png";
        spec->result = slot;
        slot->state = MaterialSlot::Loading;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadPbrMaterialWork);
    } break;
    case EntityMaterial::Checkerboard: {
        auto slot = manager->materials + (u32)EntityMaterial::Checkerboard;

        auto spec = (LegacyMaterialSpec*)PlatformAlloc(sizeof(LegacyMaterialSpec));
        *spec = {};
        spec->diffuse = "../res/checkerboard.jpg";
        spec->result = slot;
        slot->state = MaterialSlot::Loading;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadLegacyMaterialWork);
    } break;
    invalid_default();
    }
}

Mesh* Get(AssetManager* manager, EntityMesh id) {
    Mesh* result = nullptr;
    auto mesh = manager->meshes + (u32)id;
    switch (mesh->state) {
    case MeshSlot::Ready: {
        result = mesh->mesh;
    } break;
    case MeshSlot::JustLoaded: {
        auto begin = PlatformGetTimeStamp();
        RendererLoadMesh(mesh->mesh);
        auto end = PlatformGetTimeStamp();
        printf("[Info] Loaded mesh on gpu: %f ms\n", (end - begin) * 1000.0f);
        result = mesh->mesh;
        mesh->state = MeshSlot::Ready;
    } break;
    case MeshSlot::NotLoaded: {
        LoadMesh(manager, id);
    } break;
    case MeshSlot::Loading: {} break;
        invalid_default();
    }
    return result;
}

Material* Get(AssetManager* manager, EntityMaterial id) {
    Material* result = nullptr;
    auto material = &manager->materials[(u32)id];
    switch (material->state) {
    case MaterialSlot::Ready: {
        result = &material->material;
    } break;
    case MaterialSlot::JustLoaded: {
        auto begin = PlatformGetTimeStamp();
        CompleteMaterialLoad(&material->material);
        auto end = PlatformGetTimeStamp();
        printf("[Info] Loaded material on gpu: %f ms\n", (end - begin) * 1000.0f);
        result = &material->material;
        material->state = MaterialSlot::Ready;
    } break;
    case MaterialSlot::NotLoaded: {
        LoadMaterial(manager, id);
    } break;
    case MaterialSlot::Loading: {} break;
    invalid_default();
    }
    return result;
}
