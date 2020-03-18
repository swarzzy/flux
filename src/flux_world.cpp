#include "flux_world.h"
#include "flux_serialize.h"

void Update(World* world) {
    for (Entity& entity : world->entityTable) {
        if (entity.id) {
            entity.transform = Translate(entity.p) * Scale(entity.scale);
            entity.invTransform = Inverse(entity.transform).Unwrap();
        }
    }
}

// TODO: Remove context
Option<RaycastResult> Raycast(Context* context, AssetManager* manager, World* world, v3 ro, v3 rd) {
    auto result = Option<RaycastResult>::None();
    for (Entity& entity : world->entityTable) {
        v3 roMesh = (entity.invTransform * V4(ro, 1.0f)).xyz;
        v3 rdMesh = (entity.invTransform * V4(rd, 0.0f)).xyz;
        auto mesh = GetMesh(manager, entity.mesh);
        if (mesh) {
            bool hit = IntersectFast(mesh->aabb, roMesh, rdMesh, 0.0f, F32::Max);
            if (hit) {
                result = Option<RaycastResult>::Some({entity.id});
            }
        }
    }
    return result;
}

Entity* AddEntity(World* world) {
    Entity* result = nullptr;
    auto id = world->nextEntitySerialNumber;
    assert(id);
    auto entity = Add(&world->entityTable, &id);
    if (entity) {
        world->nextEntitySerialNumber++;
        *entity = {};
        entity->id = id;
        world->entityCount++;
        result = entity;
    }
    return result;
}

StoredTexture StoreTexture(AssetManager* manager, u32 id) {
    StoredTexture result = {};
    auto texture = GetTextureSlot(manager, id);
    if (texture) {
        strcpy_s(result.filename, array_count(result.filename), texture->filename);
        result.format = (u32)texture->format;
        result.wrapMode = (u32)texture->wrapMode;
        result.filter = (u32)texture->filter;
        result.range = (u32)texture->range;
    }
    return result;
}

Entity* GetEntity(World* world, u32 id) {
    return Get(&world->entityTable, &id);
}

StoredMaterial StoreMaterial(AssetManager* manager, const Material* m) {
    StoredMaterial result = {};
    result.workflow = (u32)m->workflow;
    switch (m->workflow) {
    case Material::Phong: {
        result.phong.diffuse = StoreTexture(manager, m->phong.diffuse);
        result.phong.specular = StoreTexture(manager, m->phong.specular);
    } break;
    case Material::PBRMetallic: {
        result.pbrMetallic.albedo = StoreTexture(manager, m->pbrMetallic.albedo);
        result.pbrMetallic.roughness = StoreTexture(manager, m->pbrMetallic.roughness);
        result.pbrMetallic.metallic = StoreTexture(manager, m->pbrMetallic.metallic);
        result.pbrMetallic.normals = StoreTexture(manager, m->pbrMetallic.normals);
    } break;
    case Material::PBRSpecular: {
        result.pbrSpecular.albedo = StoreTexture(manager, m->pbrSpecular.albedo);
        result.pbrSpecular.specular = StoreTexture(manager, m->pbrSpecular.specular);
        result.pbrSpecular.gloss = StoreTexture(manager, m->pbrSpecular.gloss);
        result.pbrSpecular.normals = StoreTexture(manager, m->pbrSpecular.normals);
    } break;
    case Material::PBRMetallicCustom: {
        result.pbrMetallicCustom.albedo = m->pbrMetallicCustom.albedo;
        result.pbrMetallicCustom.roughness = m->pbrMetallicCustom.roughness;
        result.pbrMetallicCustom.metallic = m->pbrMetallicCustom.metallic;
    } break;
    case Material::PhongCustom: {
        result.phongCustom.diffuse = m->phongCustom.diffuse;
        result.phongCustom.specular = m->phongCustom.specular;
    }
    invalid_default();
    }
    return result;
}

bool SaveToDisk(AssetManager* manager, World* world, const wchar_t* filename) {
    u32 fileSize = sizeof(World) + sizeof(StoredEntity) * world->entityCount;
    void* memory = PlatformAlloc(fileSize);
    defer { PlatformFree(memory); };
    auto header = (WorldFile*)memory;
    auto entities = (StoredEntity*)((byte*)memory + sizeof(WorldFile));

    header->nextEntitySerialNumber = world->nextEntitySerialNumber;
    header->entityCount = world->entityCount;
    header->firstEntityOffset = (u32)((uptr)entities - (uptr)memory);

    u32 at = 0;
    for (Entity& entity : world->entityTable) {
        assert(at < world->entityCount);
        auto out = entities + at++;
        out->id = entity.id;
        out->p = entity.p;
        out->scale = entity.scale;
        auto mesh = GetMeshSlot(manager, entity.mesh);
        // TODO: Decide what to do when mesh is null
        if (mesh) {
            strcpy_s(out->meshFileName, array_count(out->meshFileName), mesh->filename);
            out->meshFileFormat = (u32)mesh->format;
        }
        out->material = StoreMaterial(manager, &entity.material);
    }

    auto result = PlatformDebugWriteFile(filename, memory, fileSize);
    return result;
}

u32 LoadTexture(AssetManager* assetManager, StoredTexture* stored) {
    u32 result = 0;
    auto status = AddTexture(assetManager, stored->filename, (TextureFormat)stored->format, (TextureWrapMode)stored->wrapMode, (TextureFilter)stored->filter, (DynamicRange)stored->range);
    switch (status.status) {
    case AddAssetResult::AlreadyExists: {
        AssetName name;
        GetAssetName(stored->filename, &name);
        result = GetID(&assetManager->nameTable, name.name);
        assert(result);
    } break;
    case AddAssetResult::Ok: {
        result = status.id;
    } break;
    default: {
        printf("[World] Texture %s not found\n", stored->filename);
    } break;
    }
    return result;
}

u32 LoadTextureIfExist(AssetManager* assetManager, StoredTexture* stored) {
    u32 result = 0;
    if (stored->filename[0]) {
        result = LoadTexture(assetManager, stored);
    } else {
        printf("[World]: Missing texture!\n");
    }
    return result;
}

Material LoadMaterial(AssetManager* assetManager, StoredMaterial* stored) {
    Material mat = {};
    mat.workflow = (Material::Workflow)stored->workflow;
    switch (mat.workflow) {
    case Material::Phong: {
        mat.phong.diffuse = LoadTextureIfExist(assetManager, &stored->phong.diffuse);
        mat.phong.specular = LoadTextureIfExist(assetManager, &stored->phong.specular);
    } break;
    case Material::PBRMetallic: {
        mat.pbrMetallic.albedo = LoadTextureIfExist(assetManager, &stored->pbrMetallic.albedo);
        mat.pbrMetallic.roughness = LoadTextureIfExist(assetManager, &stored->pbrMetallic.roughness);
        mat.pbrMetallic.metallic = LoadTextureIfExist(assetManager, &stored->pbrMetallic.metallic);
        mat.pbrMetallic.normals = LoadTextureIfExist(assetManager, &stored->pbrMetallic.normals);
    } break;
    case Material::PBRSpecular: {
        mat.pbrSpecular.albedo = LoadTextureIfExist(assetManager, &stored->pbrSpecular.albedo);
        mat.pbrSpecular.specular = LoadTextureIfExist(assetManager, &stored->pbrSpecular.specular);
        mat.pbrSpecular.gloss = LoadTextureIfExist(assetManager, &stored->pbrSpecular.gloss);
        mat.pbrSpecular.normals = LoadTextureIfExist(assetManager, &stored->pbrSpecular.normals);
    } break;
    case Material::PBRMetallicCustom: {
        mat.pbrMetallicCustom.albedo = stored->pbrMetallicCustom.albedo;
        mat.pbrMetallicCustom.roughness = stored->pbrMetallicCustom.roughness;
        mat.pbrMetallicCustom.metallic = stored->pbrMetallicCustom.metallic;
    } break;
    case Material::PhongCustom: {
        mat.phongCustom.diffuse = stored->phongCustom.diffuse;
        mat.phongCustom.specular = stored->phongCustom.specular;
    }
    invalid_default();
    }
    return mat;
}

World* LoadWorldFromDisc(AssetManager* assetManager, const wchar_t* filename) {
    World* world = nullptr;
    auto fileSize = PlatformDebugGetFileSize(filename);
    if (fileSize) {
        auto fileBuffer = PlatformAlloc(fileSize);
        defer { PlatformFree(fileBuffer); };
        u32 bytesRead = PlatformDebugReadFile(fileBuffer, fileSize, filename);
        assert(fileSize == bytesRead);
        auto header = (WorldFile*)fileBuffer;

        // TODO: Pretty zeroed allocations
        world = (World*)PlatformAlloc(sizeof(World));
        *world = {};

        world->nextEntitySerialNumber = header->nextEntitySerialNumber;
        world->entityCount = header->entityCount;

        auto fileEntities = (StoredEntity*)((byte*)fileBuffer + header->firstEntityOffset);
        for (u32 i = 0; i < world->entityCount; i++) {
            auto stored = fileEntities + i;
            auto entry = Add(&world->entityTable, &stored->id);
            assert(entry);
            entry->id = stored->id;
            entry->p = stored->p;
            entry->scale = stored->scale;
            entry->material = LoadMaterial(assetManager, &stored->material);
            auto result = AddMesh(assetManager, stored->meshFileName, (MeshFileFormat)stored->meshFileFormat);
            switch (result.status) {
            case AddAssetResult::AlreadyExists: {
                AssetName name;
                GetAssetName(stored->meshFileName, &name);
                entry->mesh = GetID(&assetManager->nameTable, name.name);
                assert(entry->mesh);
            } break;
            case AddAssetResult::Ok: {
                entry->mesh = result.id;
            } break;
            default: {
                entry->mesh = 0;
                printf("[World] Mesh %s not found\n", stored->meshFileName);
            } break;
            }
        }
    }
    return world;
}
