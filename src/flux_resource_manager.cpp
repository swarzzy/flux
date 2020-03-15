#include "flux_resource_manager.h"
#include "flux_file_formats.h"


u32 AddName(AssetNameTable* table, const char* _name) {
    u32 result = 0;
    AssetName name;
    strcpy_s(name.name, array_count(name.name), _name);
    u32* guid = Add(&table->table, &name);
    if (guid) {
        *guid = table->serialCount++;
        result = *guid;
    }
    return result;
}

void RemoveName(AssetNameTable* table, const char* _name) {
    // TODO: Stop copying string!!!
    AssetName name;
    strcpy_s(name.name, array_count(name.name), _name);
    auto result = Delete(&table->table, &name);
    assert(result);
}

u32 GetID(AssetNameTable* table, const char* _name) {
    u32 result = 0;
    // TODO: Stop copying string!!!
    AssetName name;
    strcpy_s(name.name, array_count(name.name), _name);
    u32* id = Get(&table->table, &name);
    if (id) {
        result = *id;
    }
    return result;
}

Mesh* ReadMeshFileFlux(void* file, u32 fileSize) {
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


const char* ToString(OpenMeshResult::Result value) {
    static const char* strings[] = {
        "Unknown error",
        "Ok",
        "File name is too long",
        "File not found",
        "Read file error",
        "Invalid file format",
    };
    assert((u32)value < array_count(strings));
    return strings[(u32)value];
}

OpenMeshResult OpenMeshFileFlux(const char* filename) {
    OpenMeshResult result = {};

    if (strlen(filename) < MaxAssetPathSize) {
        wchar_t filenameW[MaxAssetPathSize];
        mbstowcs(filenameW, filename, array_count(filenameW));
        auto fileSize = PlatformDebugGetFileSize(filenameW);

        if (fileSize) {
            void* file = PlatformAlloc(fileSize);
            u32 bytesRead = PlatformDebugReadFile(file, fileSize, filenameW);

            if (bytesRead == fileSize) {

                auto header = (FluxMeshHeader*)file;
                if ((header->header.magicValue == FluxFileHeader::MagicValue) &&
                    (header->header.type == FluxFileHeader::Mesh) &&
                    (header->version == 1) &&
                    (header->entryCount > 0)) {

                    result = { OpenMeshResult::Ok, file, fileSize};
                } else {
                    PlatformFree(file);
                    result = { OpenMeshResult::InvalidFileFormat, nullptr, 0};
                }
            } else {
                PlatformFree(file);
                result = { OpenMeshResult::ReadFileError, nullptr, 0 };
            }
        } else {
            result = { OpenMeshResult::FileNotFound, nullptr, 0 };
        }
    } else {
        result = { OpenMeshResult::FileNameIsTooLong, nullptr, 0 };
    }
    return result;
}

void CloseMeshFile(void* file) {
    PlatformFree(file);
}

Mesh* LoadMeshFlux(const char* filename) {
    Mesh* result = nullptr;
    auto status = OpenMeshFileFlux(filename);
    if (status.status == OpenMeshResult::Ok) {
        result = ReadMeshFileFlux(status.file, status.fileSize);
        CloseMeshFile(status.file);
    }
    return result;
}

const u32 AAB_FILE_MAGIC_VALUE = 0xaabaabaa;
#pragma pack(push, 1)
struct AABMeshHeaderV2 {
    u32 magicValue;
    u32 version = 2;
    u64 assetSize;
    u32 assetType;

    u32 vertexCount;
    u32 indexCount;
    u64 vertexOffset;
    u64 normalsOffset;
    u64 uvOffset;
    u64 indicesOffset;
    u64 tangentsOffset;
};
#pragma pack(pop)

Mesh* ReadMeshFileAAB(void* memory, void* fileData, u32 fileSize) {
    auto mesh = (Mesh*)memory;
    *mesh = {};
    mesh->base = memory;

    auto header = (AABMeshHeaderV2*)fileData;
    assert(header->magicValue == AAB_FILE_MAGIC_VALUE);

    mesh->vertexCount = header->vertexCount;
    mesh->indexCount = header->indexCount;

    mesh->vertices = (v3*)((byte*)fileData + header->vertexOffset);
    mesh->normals = (v3*)((byte*)fileData + header->normalsOffset);
    mesh->uvs = (v2*)((byte*)fileData + header->uvOffset);
    mesh->indices = (u32*)((byte*)fileData + header->indicesOffset);
    mesh->tangents = (v3*)((byte*)fileData + header->tangentsOffset);

    mesh->aabb = BBoxAligned::From(mesh);
    return mesh;
}


OpenMeshResult OpenMeshFileAAB(const char* filename) {
    OpenMeshResult result = {};

    if (strlen(filename) < MaxAssetPathSize) {
        wchar_t filenameW[MaxAssetPathSize];
        mbstowcs(filenameW, filename, array_count(filenameW));

        auto fileSize = PlatformDebugGetFileSize(filenameW);

        if (fileSize) {

            void* fileData = PlatformAlloc(fileSize);
            u32 bytesRead = PlatformDebugReadFile(fileData, fileSize, filenameW);

            if (bytesRead == fileSize) {
                auto header = (AABMeshHeaderV2*)fileData;
                if (header->magicValue == AAB_FILE_MAGIC_VALUE) {
                    result = { OpenMeshResult::Ok, fileData, fileSize};
                } else {
                    PlatformFree(fileData);
                    result = { OpenMeshResult::InvalidFileFormat, nullptr, 0};
                }
            } else {
                PlatformFree(fileData);
                result = { OpenMeshResult::ReadFileError, nullptr, 0 };
            }
        } else {
            result = { OpenMeshResult::FileNotFound, nullptr, 0 };
        }
    } else {
        result = { OpenMeshResult::FileNameIsTooLong, nullptr, 0 };
    }
    return result;
}


Mesh* LoadMeshAAB(const char* filename) {
    Mesh* mesh = nullptr;
    // TODO: Error checking
    if (strlen(filename) < MaxAssetPathSize) {
        wchar_t filenameW[512];
        mbstowcs(filenameW, filename, array_count(filenameW));

        // Make shure loaded data will be propperly aligned
        auto headerSize = sizeof(Mesh) + CalculatePadding(sizeof(Mesh), 16);
        auto dataOffset = headerSize;

        u32 fileSize = PlatformDebugGetFileSize(filenameW);
        if (fileSize) {
            auto totalSize = fileSize + headerSize;
            void* memory = PlatformAlloc(totalSize);
            mesh = (Mesh*)memory;
            *mesh = {};
            mesh->base = memory;
            auto fileData = (void*)((byte*)memory + dataOffset);
            u32 result = PlatformDebugReadFile(fileData, fileSize, filenameW);
            if (result) {
                auto header = (AABMeshHeaderV2*)fileData;
                if (header->magicValue == AAB_FILE_MAGIC_VALUE) {
                    mesh->vertexCount = header->vertexCount;
                    mesh->indexCount = header->indexCount;

                    mesh->vertices = (v3*)((byte*)fileData + header->vertexOffset);
                    mesh->normals = (v3*)((byte*)fileData + header->normalsOffset);
                    mesh->uvs = (v2*)((byte*)fileData + header->uvOffset);
                    mesh->indices = (u32*)((byte*)fileData + header->indicesOffset);
                    mesh->tangents = (v3*)((byte*)fileData + header->tangentsOffset);

                    mesh->aabb = BBoxAligned::From(mesh);
                } else {
                    PlatformFree(memory);
                }
            } else {
                PlatformFree(memory);
            }
        }
    }
    return mesh;
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

Material LoadMaterialPBR(const char* albedoPath, const char* roughnessPath, const char* metallicPath, const char* normalsPath) {
    Texture albedo = LoadTexture(albedoPath, TextureFormat::SRGBA8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture roughness = LoadTexture(roughnessPath, TextureFormat::R8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture metallic = LoadTexture(metallicPath, TextureFormat::R8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture normals = LoadTexture(normalsPath, TextureFormat::RGB8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);

    Material material = {};
    material.workflow = Material::PBRMetallic;
    material.pbrMetallic.albedo = albedo;
    material.pbrMetallic.roughness = roughness;
    material.pbrMetallic.metallic = metallic;
    material.pbrMetallic.normals = normals;

    return material;
}

Material LoadMaterialPhong(const char* diffusePath, const char* specularPath) {
    Texture diffMap = LoadTexture(diffusePath, TextureFormat::SRGB8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    Texture specMap = {};
    if (specularPath) {
        Texture specMap = LoadTexture(specularPath, TextureFormat::SRGB8, TextureWrapMode::Repeat, TextureFilter::Anisotropic);
    }

    Material material = {};
    material.workflow = Material::Phong;
    material.phong.diffMap = diffMap;
    material.phong.specMap = specMap;

    return material;
}

void CompleteMaterialLoad(Material* material) {
    if (material->workflow == Material::PBRMetallic) {
        UploadToGPU(&material->pbrMetallic.albedo);
        UploadToGPU(&material->pbrMetallic.roughness);
        UploadToGPU(&material->pbrMetallic.metallic);
        UploadToGPU(&material->pbrMetallic.normals);
    } else if (material->workflow == Material::PBRSpecular) {
        UploadToGPU(&material->pbrSpecular.albedo);
        UploadToGPU(&material->pbrSpecular.specular);
        UploadToGPU(&material->pbrSpecular.gloss);
        UploadToGPU(&material->pbrSpecular.normals);
    } else if (material->workflow == Material::Phong) {
        UploadToGPU(&material->phong.diffMap);
        if (material->phong.specMap.data) {
            UploadToGPU(&material->phong.specMap);
        }
    }
}

AssetQueueEntry* AssetQueuePush(AssetManager* manager) {
    AssetQueueEntry* ptr = nullptr;
    if (manager->assetQueueAt < array_count(manager->assetQueue)) {
        auto index = manager->assetQueueAt++;
        ptr = manager->assetQueue + index;
    }
    return ptr;
}

void AssetQueueRemove(AssetManager* manager, u32 index) {
    assert(index < manager->assetQueueAt);
    if ((manager->assetQueueAt - 1) != index) {
        auto slot = manager->assetQueue + index;
        auto slotToMove = manager->assetQueue + manager->assetQueueAt - 1;
        *slot = *slotToMove;
        manager->assetQueueAt--;
    } else {
        manager->assetQueueAt--;
    }
}

void LoadMeshWork(void* _data, u32 threadIndex) {
    auto data = (AssetSlot<Mesh*>*)_data;
    printf("[Asset manager] Thread %d: Loading mesh %s\n", (int)threadIndex, data->filename);
    Mesh* mesh = nullptr;
    switch (data->format)
    {
    case MeshFileFormat::AAB: {
        mesh = LoadMeshAAB(data->filename);
    } break;
    case MeshFileFormat::Flux: {
        mesh = LoadMeshFlux(data->filename);
    } break;
        invalid_default();
    }
    if (mesh) {
        data->asset = mesh;
        auto prevState = AtomicExchange((u32 volatile*)&data->state, (u32)AssetState::JustLoaded);
        assert(prevState == (u32)AssetState::Queued);
    } else {
        printf("[Asset manager] Failed to load mesh: %s\n", data->filename);
        auto prevState = AtomicExchange((u32 volatile*)&data->state, (u32)AssetState::Error);
        assert(prevState == (u32)AssetState::Queued);
    }
}

void GetMeshName(const char* filename, AssetName* name) {
    // TODO: This is slow for now
    u32 len = (u32)strlen(filename);
    u32 extBeginIndex = len;
    u32 dirBeginIndex = len;
    for (i32 i = len; i >= 0; i--) {
        char at = filename[i];
        if (at == '.') {
            extBeginIndex = i;
            break;
        }
    }
    for (i32 i = len; i >= 0; i--) {
        char at = filename[i];
        if (at == '/' || at == '\\') {
            dirBeginIndex = i;
            break;
        }
    }
    u32 begin = 0;
    if (dirBeginIndex < extBeginIndex) {
        begin = dirBeginIndex + 1;
    }
    u32 end = len;
    if (extBeginIndex > dirBeginIndex) {
        end = extBeginIndex;
    }

    u32 count = end - begin;

    strncpy_s(name->name, array_count(name->name), filename + begin, count);
}

AddMeshResult AddMesh(AssetManager* manager, const char* filename, MeshFileFormat format) {
    AddMeshResult result = {};
    OpenMeshResult fileStatus = {};
    switch (format) {
    case MeshFileFormat::AAB: { fileStatus = OpenMeshFileAAB(filename); } break;
    case MeshFileFormat::Flux: { fileStatus = OpenMeshFileFlux(filename); } break;
    invalid_default();
    }

    if (fileStatus.status == OpenMeshResult::Ok) {
        CloseMeshFile(fileStatus.file);
        AssetName name;
        GetMeshName(filename, &name);
        bool alreadyExists = GetID(&manager->nameTable, name.name) != 0;
        if (!alreadyExists) {
            u32 id = AddName(&manager->nameTable, name.name);
            assert(id);
            auto slot = Add(&manager->meshTable, &id);
            assert(slot);
            slot->id = id;
            strcpy_s(slot->name, array_count(slot->name), name.name);
            strcpy_s(slot->filename, array_count(slot->filename), filename);
            slot->format = format;
            result = { AddMeshResult::Ok, id };
        } else {
            printf("[Asset manager] Failed to load asset %s. An asset with the same name is already loaded.\n", filename);
            result = { AddMeshResult::AlreadyExists, 0 };
        }
    } else {
        printf("[Asset manager] Failed to open asset file: %s. Error: %s\n", filename, ToString(fileStatus.status));
    }
    return result;
}

void LoadMesh(AssetManager* manager, u32 id) {
    auto slot = Get(&manager->meshTable, &id);
    if (slot) {
        auto queueEntry = AssetQueuePush(manager);
        if (queueEntry) {
            queueEntry->id = id;
            queueEntry->type = AssetType::Mesh;

            PlatformPushWork(GlobalPlaformWorkQueue, slot, LoadMeshWork);
        }
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
    printf("[Asset manager] Thread %d: Loading material %s\n", (int)threadIndex, spec->albedo);
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
    printf("[Asset manager] Thread %d: Loading material %s\n", (int)threadIndex, spec->diffuse);
    auto material = LoadMaterialPhong(spec->diffuse, spec->specular);
    spec->result->asset = material;
    // NOTE: AtomicExchange provides rw fence here
    auto prevState = AtomicExchange((u32 volatile*)&spec->result->state, (u32)AssetState::JustLoaded);
    assert(prevState == (u32)AssetState::Queued);
    PlatformFree(spec);
}

void LoadMaterial(AssetManager* manager, EntityMaterial id) {
    auto queueEntry = AssetQueuePush(manager);
    if (queueEntry) {
        queueEntry->id = (u32)id;
        queueEntry->type = AssetType::Material;

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
    } else {
        printf("[Asset manager] Unable to queue asset load. Queue is full\n");
    }
}

void CompleteMeshLoad(AssetManager* manager, u32 queueIndex, u32 id) {
    auto slot = Get(&manager->meshTable, &id);
    assert(slot);
    assert(slot->state == AssetState::Queued || slot->state == AssetState::JustLoaded || slot->state == AssetState::Error);
    assert(manager->assetQueue[queueIndex].id == (u32)id);
    if (slot->state == AssetState::JustLoaded) {
        AssetQueueRemove(manager, queueIndex);
        auto begin = PlatformGetTimeStamp();
        UploadToGPU(slot->asset);
        auto end = PlatformGetTimeStamp();
        printf("[Asset manager] Loaded material on gpu: %f ms\n", (end - begin) * 1000.0f);
        slot->state = AssetState::Loaded;
    } else if (slot->state == AssetState::Error) {
        AssetQueueRemove(manager, queueIndex);
#if 0
        // TODO: Clean this up
        auto slot = GetMeshSlot(manager, id);
        assert(slot);
        AssetName name;
        strcpy_s(name.name, array_count(name.name), slot->name);
        Delete(&manager->nameTable.table, &name);
        Delete(&manager->meshTable, &id);
#endif
    }
}

void CompleteAssetLoad(AssetManager* manager, u32 queueIndex, EntityMaterial id) {
    auto slot = manager->materials + (u32)id;
    assert(slot->state == AssetState::Queued || slot->state == AssetState::JustLoaded || slot->state == AssetState::Error);
    if (slot->state == AssetState::JustLoaded) {
        assert(manager->assetQueue[queueIndex].id == (u32)id);
        AssetQueueRemove(manager, queueIndex);
        auto begin = PlatformGetTimeStamp();
        CompleteMaterialLoad(&slot->asset);
        auto end = PlatformGetTimeStamp();
        printf("[Asset manager] Loaded mesh on gpu: %f ms\n", (end - begin) * 1000.0f);
        slot->state = AssetState::Loaded;
    }
}

void CompletePendingLoads(AssetManager* manager) {
    // TODO: Store asset state in queue entry
    for (u32 i = 0; i < manager->assetQueueAt; i++) {
        auto entry = manager->assetQueue + i;
        switch (entry->type) {
        case AssetType::Material: { CompleteAssetLoad(manager, i, (EntityMaterial)entry->id); } break;
        case AssetType::Mesh: { CompleteMeshLoad(manager, i, entry->id); } break;
            invalid_default();
        }
    }
}

AssetSlot<Mesh*>* GetMeshSlot(AssetManager* manager, u32 id) {
    return Get(&manager->meshTable, &id);
}

Mesh* GetMesh(AssetManager* manager, u32 id) {
    Mesh* result = nullptr;
    auto mesh = Get(&manager->meshTable, &id);
    if (mesh) {
        switch (mesh->state) {
        case AssetState::Loaded: {
            result = mesh->asset;
        } break;
        case AssetState::Unloaded: {
            mesh->state = AssetState::Queued;
            LoadMesh(manager, id);
        } break;
        case AssetState::JustLoaded: {} break;
        case AssetState::Queued: {} break;
        case AssetState::Error: {} break;
            invalid_default();
        }
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
    case AssetState::Unloaded: {
        material->state = AssetState::Queued;
        LoadMaterial(manager, id);
    } break;
    case AssetState::JustLoaded: {} break;
    case AssetState::Queued: {} break;
    invalid_default();
    }
    return result;
}
