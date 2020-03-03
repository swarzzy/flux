#pragma once

struct Entity {
    u32 id;
    v3 p;
    v3 scale = V3(1.0f);
    Mesh* mesh;
    Material* material;
    m4x4 transform;
    m4x4 invTransform;
};

// TODO: Entity iterators
struct World {
    u32 nextEntitySerialNumber = 1;
    Entity entities[32];
};

struct RaycastResult {
    u32 entityId;
};

void Update(World* world);
Option<RaycastResult> Raycast(World* world, v3 ro, v3 rd);
Entity* AddEntity(World* world);
Entity* GetEntity(World* world, u32 id);
bool DeleteEntity(World* world, u32 id);
