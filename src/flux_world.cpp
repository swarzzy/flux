#include "flux_world.h"

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
        auto mesh = GetMesh(manager, context->meshIDs[entity.mesh]);
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
    auto entity = Add(&world->entityTable, id);
    if (entity) {
        world->nextEntitySerialNumber++;
        *entity = {};
        entity->id = id;
        world->entityCount++;
        result = entity;
    }
    return result;
}

bool SaveToDisk(World* world, const wchar_t* filename) {
    u32 fileSize = sizeof(World) + sizeof(StoredEntity) * world->entityCount;
    void* memory = PlatformAlloc(fileSize);
    defer { PlatformFree(memory); };
    auto header = (WorldFile*)memory;
    auto entities = (StoredEntity*)((byte*)memory + sizeof(StoredEntity));

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
        out->meshId = (u32)entity.mesh;
        out->materialId = (u32)entity.material;
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
            auto entry = Add(&world->entityTable, stored->id);
            assert(entry);
            entry->id = stored->id;
            entry->p = stored->p;
            entry->scale = stored->scale;
            entry->mesh = stored->meshId;
            entry->material = (EntityMaterial)stored->materialId;
        }
    }
    return world;
}
