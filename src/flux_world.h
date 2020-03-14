#pragma once
#include "flux_resource_manager.h"
#include "flux_hash_map.h"

struct Entity {
    u32 id;
    v3 p;
    v3 scale = V3(1.0f);
    u32 mesh;
    EntityMaterial material;
    m4x4 transform;
    m4x4 invTransform;
};

// TODO: Entity iterators
struct World {
    u32 nextEntitySerialNumber = 1;
    u32 entityCount;
    HashMap<Entity> entityTable;
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
Option<RaycastResult> Raycast(Context* context, AssetManager* manager, World* world, v3 ro, v3 rd);
Entity* AddEntity(World* world);
bool SaveToDisk(World* world, const wchar_t* filename);
World* LoadWorldFromDisc(const wchar_t* filename);
