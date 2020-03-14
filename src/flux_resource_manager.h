#pragma once

#include "flux_platform.h"
#include "flux_memory.h"
#include "flux_hash_map.h"

struct AssetNameTable {
    u32 serialCount = 1;
    HashMap<u32> table;
};

u32 AddName(AssetNameTable* table, u32 nameHash);
void RemoveName(AssetNameTable* table, u32 nameHash);
u32 GetID(AssetNameTable* table, const u32 nameHash);

u32 AddName(AssetNameTable* table, const char* name);
void RemoveName(AssetNameTable* table, const char* name);
u32 GetID(AssetNameTable* table, const char* name);


enum struct TextureFilter : u32 {
    None, Bilinear, Trilinear, Anisotropic, Default = Bilinear
};

enum struct TextureFormat : u32 {
    Unknown, SRGBA8, SRGB8, RGBA8, RGB8, RGB16F, RG16F, R8, RG8
};

enum struct TextureWrapMode : u32 {
    Repeat, ClampToEdge, Default = Repeat
};

struct Texture {
    TextureFilter filter;
    TextureFormat format;
    TextureWrapMode wrapMode;
    DynamicRange range;
    u32 width;
    u32 height;
    void* data;
    u32 gpuHandle;
};

struct CubeTexture {
    GLuint gpuHandle;
    b32 useMips;
    TextureFilter filter;
    TextureFormat format;
    TextureWrapMode wrapMode;
    u32 width;
    u32 height;
    union {
        void* data[6];
        struct {
            void* rightData;
            void* leftData;
            void* upData;
            void* downData;
            void* frontData;
            void* backData;
        };
    };
};

struct Material {
    enum  { Phong = 0, PBRMetallic, PBRSpecular, PBRMetallicCustom } workflow;
    union {
        struct {
            Texture diffMap;
            Texture specMap;
        } phong;
        struct {
            Texture albedo;
            Texture roughness;
            Texture metallic;
            Texture normals;
        } pbrMetallic;
        struct {
            Texture albedo;
            Texture specular;
            Texture gloss;
            Texture normals;
        } pbrSpecular;
        struct {
            v3 albedo;
            f32 roughness;
            f32 metalness;
        } pbrMetallicCustom;
    };
};


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
    Gizmos,
    _Count
};

enum struct AssetState : u32 {
    Unloaded = 0, Queued, JustLoaded, Loaded
};

enum struct MeshFileFormat {
    AAB, Flux
};

template<typename T>
struct AssetSlot {
    volatile AssetState state;
    T asset;
    char filename[128];
    MeshFileFormat format;
};

enum struct AssetType {
    Material, Mesh
};

struct AssetQueueEntry {
    AssetType type;
    u32 id;
};

struct AssetManager {
    AssetNameTable nameTable;
    HashMap<AssetSlot<Mesh*>> meshTable;
    AssetSlot<Mesh*> meshes[EntityMesh::_Count];
    AssetSlot<Material> materials[EntityMaterial::_Count];
    u32 assetQueueAt;
    AssetQueueEntry assetQueue[32];
};

u32 AddMesh(AssetManager* manager, const char* filename, MeshFileFormat format);

Mesh* GetMesh(AssetManager* manager, u32 id);
Material* Get(AssetManager* manager, EntityMaterial id);

void CompletePendingLoads(AssetManager* manager);

Mesh* LoadMeshFlux(const char* filename);
Mesh* LoadMeshAAB(const char* filename);

Material LoadMaterialPBRMetallic(const char* albedoPath, const char* roughnessPath, const char* metalnessPath, const char* normalsPath);
Material LoadMaterialPhong(const char* diffusePath, const char* specularPath = nullptr);
void CompleteMaterialLoad(Material* material);

Texture LoadTexture(const char* filename, TextureFormat format = TextureFormat::Unknown, TextureWrapMode wrapMode = TextureWrapMode::Default, TextureFilter filter = TextureFilter::Default, DynamicRange range = DynamicRange::LDR);
Texture CreateTexture(i32 width, i32 height, TextureFormat format, TextureWrapMode wrapMode, TextureFilter filter, void* data = 0);
CubeTexture LoadCubemap(const char* backPath, const char* downPath, const char* frontPath, const char* leftPath, const char* rightPath, const char* upPath, DynamicRange range = DynamicRange::LDR, TextureFormat format = TextureFormat::Unknown, TextureFilter filter = TextureFilter::Default, TextureWrapMode wrapMode = TextureWrapMode::Default);
CubeTexture MakeEmptyCubemap(u32 w, u32 h, TextureFormat format, TextureFilter filter, bool useMips);

inline CubeTexture LoadCubemapLDR(const char* backPath, const char* downPath, const char* frontPath,
                               const char* leftPath, const char* rightPath, const char* upPath) {
    return LoadCubemap(backPath, downPath, frontPath, leftPath, rightPath, upPath, DynamicRange::LDR, TextureFormat::SRGB8);
}

inline CubeTexture LoadCubemapHDR(const char* backPath, const char* downPath, const char* frontPath,
                                  const char* leftPath, const char* rightPath, const char* upPath) {
    return LoadCubemap(backPath, downPath, frontPath, leftPath, rightPath, upPath, DynamicRange::HDR, TextureFormat::RGB16F);
}
