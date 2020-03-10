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

int STBDesiredBPPFromTextureFormat(TextureFormat format) {
    int desiredBpp = 0;
    switch (format) {
    case TextureFormat::SRGBA8:
    case TextureFormat::RGBA8: { desiredBpp = 4; } break;
    case TextureFormat::SRGB8:
    case TextureFormat::RGB8:
    case TextureFormat::RGB16F: { desiredBpp = 3; } break;
    case TextureFormat::RG16F: { desiredBpp = 2; } break;
    case TextureFormat::R8: { desiredBpp = 1; } break;
        invalid_default();
    }
    return desiredBpp;
}

TextureFormat GuessTexFormatFromNumChannels(u32 num) {
    TextureFormat format;
    switch (num) {
    case 1: { format = TextureFormat::R8; } break;
    case 2: { format = TextureFormat::RG8; } break; // TODO: Implement in renderer
    case 3: { format = TextureFormat::RGB8; } break;
    case 4: { format = TextureFormat::SRGBA8; } break;
        invalid_default();
    }
    return format;
}

Texture LoadTexture(const char* filename, TextureFormat format, TextureWrapMode wrapMode, TextureFilter filter, DynamicRange range) {
    Texture t = {};

    i32 desiredBpp = 0;

    if (format != TextureFormat::Unknown) {
        desiredBpp = STBDesiredBPPFromTextureFormat(format);
    }

    auto image = ResourceLoaderLoadImage(filename, range, true, desiredBpp, PlatformAlloc);
    assert(image);

    if (format == TextureFormat::Unknown) {
        format = GuessTexFormatFromNumChannels(image->channels);
    }

    t.format = format;
    t.width = image->width;
    t.height = image->height;
    t.wrapMode = wrapMode;
    t.filter = filter;
    t.data = image->bits;

    return t;
}

Texture CreateTexture(i32 width, i32 height, TextureFormat format, TextureWrapMode wrapMode, TextureFilter filter, void* data) {
    Texture t = {};

    t.format = format;
    t.width = width;
    t.height = height;
    t.filter = filter;
    t.wrapMode = wrapMode;
    t.data = data;

    return t;
}

CubeTexture LoadCubemap(const char* backPath, const char* downPath, const char* frontPath,
                        const char* leftPath, const char* rightPath, const char* upPath,
                        DynamicRange range, TextureFormat format, TextureFilter filter, TextureWrapMode wrapMode) {
    CubeTexture texture = {};

    // TODO: Use memory arena
    // TODO: Free memory
    auto back = ResourceLoaderLoadImage(backPath, range, false, 0, PlatformAlloc);
    //defer { PlatformFree(back->base); };
    auto down = ResourceLoaderLoadImage(downPath, range, false, 0, PlatformAlloc);
    //defer { PlatformFree(down->base); };
    auto front = ResourceLoaderLoadImage(frontPath, range, false, 0, PlatformAlloc);
    //defer { PlatformFree(front->base); };
    auto left = ResourceLoaderLoadImage(leftPath, range, false, 0, PlatformAlloc);
    //defer { PlatformFree(left->base); };
    auto right = ResourceLoaderLoadImage(rightPath, range, false, 0, PlatformAlloc);
    //defer { PlatformFree(right->base); };
    auto up = ResourceLoaderLoadImage(upPath, range, false, 0, PlatformAlloc);
    //defer { PlatformFree(up->base); };

    assert(back->width == down->width);
    assert(back->width == front->width);
    assert(back->width == left->width);
    assert(back->width == right->width);
    assert(back->width == up->width);

    assert(back->height == down->height);
    assert(back->height == front->height);
    assert(back->height == left->height);
    assert(back->height == right->height);
    assert(back->height == up->height);

    texture.format = format;
    texture.width = back->width;
    texture.height = back->height;
    texture.backData = back->bits;
    texture.downData = down->bits;
    texture.frontData = front->bits;
    texture.leftData = left->bits;
    texture.rightData = right->bits;
    texture.upData = up->bits;

    return texture;
}

CubeTexture MakeEmptyCubemap(u32 w, u32 h, TextureFormat format, TextureFilter filter, bool useMips) {
    CubeTexture texture = {};
    texture.useMips = useMips;
    texture.filter = filter;
    texture.format = format;
    texture.width = w;
    texture.height = h;
    return texture;
}

Material LoadMaterialPBR(const char* albedoPath, const char* roughnessPath, const char* metalnessPath, const char* normalsPath) {
    Texture albedo = LoadTexture(albedoPath, TextureFormat::SRGBA8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture roughness = LoadTexture(roughnessPath, TextureFormat::R8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture metalness = LoadTexture(metalnessPath, TextureFormat::R8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture normals = LoadTexture(normalsPath, TextureFormat::RGB8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);

    Material material = {};
    material.type = Material::Type::PBR;
    material.workflow = Material::Workflow::Metallic;
    material.pbr.metallic.albedo = albedo;
    material.pbr.metallic.roughness = roughness;
    material.pbr.metallic.metalness = metalness;
    material.pbr.metallic.normals = normals;

    return material;
}

Material LoadMaterialLegacy(const char* diffusePath, const char* specularPath) {
    Texture diffMap = LoadTexture(diffusePath, TextureFormat::SRGB8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture specMap = {};
    if (specularPath) {
        Texture specMap = LoadTexture(specularPath, TextureFormat::SRGB8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    }

    Material material = {};
    material.type = Material::Type::Legacy;
    material.legacy.diffMap = diffMap;
    material.legacy.specMap = specMap;

    return material;
}

void CompleteMaterialLoad(Material* material) {
    if (material->type == Material::Type::PBR) {
        UploadToGPU(&material->pbr.metallic.albedo);
        UploadToGPU(&material->pbr.metallic.roughness);
        UploadToGPU(&material->pbr.metallic.metalness);
        UploadToGPU(&material->pbr.metallic.normals);
    } else {
        UploadToGPU(&material->legacy.diffMap);
        if (material->legacy.specMap.data) {
            UploadToGPU(&material->legacy.specMap);
        }
    }
}

// TODO: Asset names encoding
struct LoadMeshWorkData {
    const wchar_t* filename;
    enum struct MeshType {AAB, Flux} meshType;
    AssetSlot<Mesh*>* result;
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

    data->result->asset = mesh;

    auto prevState = AtomicExchange((u32 volatile*)&data->result->state, (u32)AssetState::JustLoaded);
    assert(prevState == (u32)AssetState::Queued);
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

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadMeshWork);
    } break;
    case EntityMesh::Sphere: {
        auto slot = manager->meshes + (u32)EntityMesh::Sphere;
        auto spec = (LoadMeshWorkData*)PlatformAlloc(sizeof(LoadMeshWorkData));
        *spec = {};
        spec->filename = L"../res/meshes/sphere.aab";
        spec->meshType = LoadMeshWorkData::MeshType::AAB;
        spec->result = slot;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadMeshWork);
    } break;
    case EntityMesh::Plate: {

        auto slot = manager->meshes + (u32)EntityMesh::Plate;
        auto spec = (LoadMeshWorkData*)PlatformAlloc(sizeof(LoadMeshWorkData));
        *spec = {};
        spec->filename = L"../res/meshes/plate.aab";
        spec->meshType = LoadMeshWorkData::MeshType::AAB;
        spec->result = slot;

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
    AssetSlot<Material>* result;
};

struct LegacyMaterialSpec {
    const char* diffuse;
    const char* specular;
    AssetSlot<Material>* result;
};

void LoadPbrMaterialWork(void* materialSpec, u32 threadIndex) {
    //PlatformSleep(1000);
    auto spec = (PbrMaterialSpec*)materialSpec;
    printf("[Info] Thread %d: Loading material %s\n", (int)threadIndex, spec->albedo);
    auto material = LoadMaterialPBR(spec->albedo, spec->roughness, spec->metallic, spec->normals);
    spec->result->asset = material;
    // NOTE: AtomicExchange provides rw fence here
    auto prevState = AtomicExchange((u32 volatile*)&spec->result->state, (u32)AssetState::JustLoaded);
    assert(prevState == (u32)AssetState::Queued);
    // TODO: Use arenas for this allocations
    PlatformFree(spec);
}

void LoadLegacyMaterialWork(void* materialSpec, u32 threadIndex) {
    //PlatformSleep(1000);
    auto spec = (LegacyMaterialSpec*)materialSpec;
    printf("[Info] Thread %d: Loading material %s\n", (int)threadIndex, spec->diffuse);
    auto material = LoadMaterialLegacy(spec->diffuse, spec->specular);
    spec->result->asset = material;
    // NOTE: AtomicExchange provides rw fence here
    auto prevState = AtomicExchange((u32 volatile*)&spec->result->state, (u32)AssetState::JustLoaded);
    assert(prevState == (u32)AssetState::Queued);
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

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadPbrMaterialWork);
    } break;
    case EntityMaterial::Checkerboard: {
        auto slot = manager->materials + (u32)EntityMaterial::Checkerboard;

        auto spec = (LegacyMaterialSpec*)PlatformAlloc(sizeof(LegacyMaterialSpec));
        *spec = {};
        spec->diffuse = "../res/checkerboard.jpg";
        spec->result = slot;

        PlatformPushWork(GlobalPlaformWorkQueue, spec, LoadLegacyMaterialWork);
    } break;
    invalid_default();
    }
}

Mesh* Get(AssetManager* manager, EntityMesh id) {
    Mesh* result = nullptr;
    auto mesh = manager->meshes + (u32)id;
    switch (mesh->state) {
    case AssetState::Loaded: {
        result = mesh->asset;
    } break;
    case AssetState::JustLoaded: {
        auto begin = PlatformGetTimeStamp();
        UploadToGPU(mesh->asset);
        auto end = PlatformGetTimeStamp();
        printf("[Info] Loaded mesh on gpu: %f ms\n", (end - begin) * 1000.0f);
        result = mesh->asset;
        mesh->state = AssetState::Loaded;
    } break;
    case AssetState::Unloaded: {
        mesh->state = AssetState::Queued;
        LoadMesh(manager, id);
    } break;
    case AssetState::Queued: {} break;
        invalid_default();
    }
    return result;
}

Material* Get(AssetManager* manager, EntityMaterial id) {
    Material* result = nullptr;
    auto material = &manager->materials[(u32)id];
    switch (material->state) {
    case AssetState::Loaded: {
        result = &material->asset;
    } break;
    case AssetState::JustLoaded: {
        auto begin = PlatformGetTimeStamp();
        CompleteMaterialLoad(&material->asset);
        auto end = PlatformGetTimeStamp();
        printf("[Info] Loaded material on gpu: %f ms\n", (end - begin) * 1000.0f);
        result = &material->asset;
        material->state = AssetState::Loaded;
    } break;
    case AssetState::Unloaded: {
        material->state = AssetState::Queued;
        LoadMaterial(manager, id);
    } break;
    case AssetState::Queued: {} break;
    invalid_default();
    }
    return result;
}
