#pragma once

enum struct EntityMaterial {
    Checkerboard = 0,
    OldMetal,
    Backpack,
    _Count
};

enum struct EntityMesh {
    Sphere = 0,
    Plate,
    Backpack,
    _Count
};

struct Entity {
    u32 id;
    v3 p;
    v3 scale = V3(1.0f);
    EntityMesh mesh;
    EntityMaterial material;
    m4x4 transform;
    m4x4 invTransform;
};

// TODO: Entity iterators
struct World {
    u32 nextEntitySerialNumber = 1;
    u32 entityCount;
    Entity entities[32];
    char name[128];
};

struct StoredEntity {
    u32 id;
    v3 p;
    v3 scale;
    u32 meshId;
    u32 materialId;
};

struct WorldFile {
    u32 nextEntitySerialNumber;
    u32 entityCount;
    u32 firstEntityOffset;
};

struct RaycastResult {
    u32 entityId;
};

struct Context ;

void Update(World* world);
Option<RaycastResult> Raycast(Context* context, World* world, v3 ro, v3 rd);
Entity* AddEntity(World* world);
Entity* GetEntity(World* world, u32 id);
bool DeleteEntity(World* world, u32 id);
bool SaveToDisk(const World* world, const wchar_t* filename);
World* LoadWorldFromDisc(const wchar_t* filename);
