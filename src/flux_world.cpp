#include "flux_world.h"

void Update(World* world) {
    for (uint i = 0; i < array_count(world->entities); i++) {
        auto entity = world->entities + i;
        if (entity->id) {
            entity->transform = Translate(entity->p) * Scale(entity->scale);
            entity->invTransform = Inverse(entity->transform).Unwrap();
        }
    }
}

// TODO: Remove context
Option<RaycastResult> Raycast(AssetManager* manager, World* world, v3 ro, v3 rd) {
    auto result = Option<RaycastResult>::None();
    for (uint i = 0; i < array_count(world->entities); i++) {
        auto entity = world->entities + i;
        if (entity->id) {
            v3 roMesh = (entity->invTransform * V4(ro, 1.0f)).xyz;
            v3 rdMesh = (entity->invTransform * V4(rd, 0.0f)).xyz;
            auto mesh = Get(manager, entity->mesh);
            if (mesh) {
                bool hit = IntersectFast(mesh->aabb, roMesh, rdMesh, 0.0f, F32::Max);
                if (hit) {
                    result = Option<RaycastResult>::Some({entity->id});
                }
            }
        }
    }
    return result;
}

Entity* FindEntry(World* world, u32 id, bool searchForEmpty) {
    Entity* result = nullptr;
    u32 cmp = searchForEmpty ? 0 : id;
    u32 hashMask = array_count(world->entities) - 1;
    auto hash = id & hashMask;
    for (u32 offset = 0; offset < array_count(world->entities); offset++) {
        u32 index = (hash + offset) & hashMask;
        auto entry = world->entities + index;
        if (entry->id == cmp) {
            result = entry;
            break;
        }
    }
    return result;
}

Entity* AddEntity(World* world) {
    Entity* result = nullptr;
    auto id = world->nextEntitySerialNumber;
    auto entry = FindEntry(world, id, true);
    if (entry) {
        world->nextEntitySerialNumber++;
        *entry = {};
        entry->id = id;
        world->entityCount++;
        result = entry;
    }
    return result;
}

Entity* GetEntity(World* world, u32 id) {
    Entity* result = nullptr;
    if (id) {
        auto entry = FindEntry(world, id, false);
        if (entry) {
            result = entry;
        }
    }
    return result;
}

bool DeleteEntity(World* world, u32 id) {
    bool result = false;
    if (id) {
        auto entry = FindEntry(world, id, false);
        if (entry) {
            assert(world->entityCount);
            world->entityCount--;
            entry->id = 0;
            result = true;
        }
    }
    return result;
}

bool SaveToDisk(const World* world, const wchar_t* filename) {
    u32 fileSize = sizeof(World) + sizeof(StoredEntity) * world->entityCount;
    void* memory = PlatformAlloc(fileSize);
    defer { PlatformFree(memory); };
    auto header = (WorldFile*)memory;
    auto entities = (StoredEntity*)((byte*)memory + sizeof(StoredEntity));

    header->nextEntitySerialNumber = world->nextEntitySerialNumber;
    header->entityCount = world->entityCount;
    header->firstEntityOffset = (u32)((uptr)entities - (uptr)memory);

    u32 at = 0;
    for (u32 i = 0; i < array_count(world->entities); i++) {
        auto entity = world->entities + i;
        if (entity->id) {
            assert(at < world->entityCount);
            auto out = entities + at++;
            out->id = entity->id;
            out->p = entity->p;
            out->scale = entity->scale;
            out->meshId = (u32)entity->mesh;
            out->materialId = (u32)entity->material;
        }
    }

    auto result = PlatformDebugWriteFile(filename, memory, fileSize);
    return result;
}

World* LoadWorldFromDisc(const wchar_t* filename) {
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
        memset(world, 0, sizeof(World));

        world->nextEntitySerialNumber = header->nextEntitySerialNumber;
        world->entityCount = header->entityCount;

        auto fileEntities = (StoredEntity*)((byte*)fileBuffer + header->firstEntityOffset);
        for (u32 i = 0; i < world->entityCount; i++) {
            auto stored = fileEntities + i;
            auto entry = FindEntry(world, stored->id, true);
            assert(entry);
            entry->id = stored->id;
            entry->p = stored->p;
            entry->scale = stored->scale;
            entry->mesh = (EntityMesh)stored->meshId;
            entry->material = (EntityMaterial)stored->materialId;
        }
    }
    return world;
}
