#pragma once
#include "flux_resource_manager.h"
#include "flux_hash_map.h"

enum struct NormalFormat {
    OpenGL = 0, DirectX
};

const char* ToString(NormalFormat value) {
    static const char* strings[] = {
        "OpenGL",
        "DirectX"
    };
    assert((u32)value < array_count(strings));
    return strings[(u32)value];
}

struct Material {
    enum  Workflow : u32 { Phong = 0, PBRMetallic, PBRSpecular, PBRMetallicCustom, PhongCustom } workflow;
    union {
        struct {
            u32 diffuse;
            u32 specular;
        } phong;
        struct {
            u32 albedo;
            u32 roughness;
            u32 metallic;
            u32 normals;
            NormalFormat normalFormat;
        } pbrMetallic;
        struct {
            u32 albedo;
            u32 specular;
            u32 gloss;
            u32 normals;
            NormalFormat normalFormat;
        } pbrSpecular;
        struct {
            v3 albedo;
            f32 roughness;
            f32 metallic;
        } pbrMetallicCustom;
        struct {
            v3 diffuse;
            v3 specular;
        } phongCustom;
    };
};

const char* ToString(Material::Workflow value) {
    switch (value) {
    case Material::Phong: { return "Phong"; } break;
    case Material::PBRMetallic: { return "PBR metallic"; } break;
    case Material::PBRSpecular: { return "PBR specular"; } break;
    case Material::PBRMetallicCustom: { return "PBR custom metallic"; } break;
    case Material::PhongCustom: { return "Phong custom"; } break;
    invalid_default();
    }
    return "";
}

struct Entity {
    u32 id;
    v3 p;
    v3 scale = V3(1.0f);
    u32 mesh;
    Material material;
    m4x4 transform;
    m4x4 invTransform;
};

// TODO: Entity iterators
struct World {
    static u32 Hasher(void* key) { return *((u32*)key); }
    static bool Comparator(void* a, void* b) { return *((u32*)a) == *((u32*)b); }
    u32 nextEntitySerialNumber = 1;
    u32 entityCount;
    HashMap<u32, Entity, Hasher, Comparator> entityTable = HashMap<u32, Entity, Hasher, Comparator>::Make();
    char name[WorldNameSize];
};


struct RaycastResult {
    u32 entityId;
};

struct Context ;

void Update(World* world);
Option<RaycastResult> Raycast(Context* context, AssetManager* manager, World* world, v3 ro, v3 rd);
Entity* AddEntity(World* world);
Entity* GetEntity(World* world, u32 id);
bool SaveToDisk(AssetManager* assetManager, World* world, const wchar_t* filename);
World* LoadWorldFromDisc(AssetManager* assetManager, const wchar_t* filename);
