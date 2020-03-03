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

Option<RaycastResult> Raycast(World* world, v3 ro, v3 rd) {
    auto result = Option<RaycastResult>::None();
    for (uint i = 0; i < array_count(world->entities); i++) {
        auto entity = world->entities + i;
        if (entity->id) {
            v3 roMesh = (entity->invTransform * V4(ro, 1.0f)).xyz;
            v3 rdMesh = (entity->invTransform * V4(rd, 0.0f)).xyz;
            bool hit = IntersectFast(entity->mesh->aabb, roMesh, rdMesh, 0.0f, F32::Max);
            if (hit) {
                result = Option<RaycastResult>::Some({entity->id});
            }
        }
    }
    return result;
}

Entity* FindEntry(World* world, u32 id) {
    Entity* result = nullptr;
    u32 hashMask = array_count(world->entities) - 1;
    auto hash = id & hashMask;
    for (u32 offset = 0; offset < array_count(world->entities); offset++) {
        u32 index = (hash + offset) & hashMask;
        auto entry = world->entities + index;
        if (entry->id == id) {
            result = entry;
            break;
        }
    }
    return result;
}

Entity* AddEntity(World* world) {
    Entity* result = nullptr;
    auto entry = FindEntry(world, 0);
    if (entry) {
        auto id = world->nextEntitySerialNumber++;
        *entry = {};
        entry->id = id;
        result = entry;
    }
    return result;
}

Entity* GetEntity(World* world, u32 id) {
    Entity* result = nullptr;
    if (id) {
        auto entry = FindEntry(world, id);
        if (entry) {
            result = entry;
        }
    }
    return result;
}

bool DeleteEntity(World* world, u32 id) {
    bool result = false;
    if (id) {
        auto entry = FindEntry(world, id);
        if (entry) {
            entry->id = 0;
            result = true;
        }
    }
    return result;
}
