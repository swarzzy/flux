#pragma once

#include "flux_platform.h"
#include "flux_memory.h"
#include "flux_hash_map.h"
#include "flux_globals.h"

struct AssetName {
    char name[MaxAssetNameSize];
};

struct AssetNameTable {
    static u32 Hash(AssetName* name) {
        // TODO: Better hash
        const char* at = name->name;
        u32 hash = 0;
        while (*at) {
            hash += *at * 31;
            at++;
        }
        return hash;
    }

    static bool Comp(AssetName* a, AssetName* b) {
        bool result = false;
        if (strcmp(a->name, b->name) == 0) {
            result = true;
        }
        return result;
    }

    u32 serialCount = 1;
    HashMap<AssetName, u32> table = HashMap<AssetName, u32>::Make(Hash, Comp);
};

// TODO: Pass manager instead of table for convinience
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

enum struct AssetState : u32 {
    Unloaded = 0, Queued, JustLoaded, Loaded, Error
};

inline const char* ToString(AssetState state) {
    static const char* strings[] = {
        "Unloaded",
        "Queued",
        "JustLoaded",
        "Loaded",
        "Error",
    };
    assert((u32)state < array_count(strings));
    return strings[(u32)state];
}

enum struct MeshFileFormat {
    AAB, Flux
};

inline const char* ToString(MeshFileFormat value) {
    static const char* strings[] = {
        "AAB",
        "Flux",
    };
    assert((u32)value < array_count(strings));
    return strings[(u32)value];
}

template<typename T>
struct AssetSlot {
    volatile AssetState state;
    T asset;
    u32 id;
    char filename[MaxAssetPathSize];
    char name[MaxAssetNameSize];
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
    static u32 Hasher(u32* key) { return *key; }
    static bool Comparator(u32* a, u32* b) { return *a == *b; }
    AssetNameTable nameTable;
    HashMap<u32, AssetSlot<Mesh*>> meshTable = HashMap<u32, AssetSlot<Mesh*>>::Make(Hasher, Comparator);
    AssetSlot<Material> materials[EntityMaterial::_Count];
    u32 assetQueueAt;
    AssetQueueEntry assetQueue[32];
};

struct AddMeshResult {
    enum {UnknownError = 0, Ok, AlreadyExists} status;
    u32 id;
};


void GetMeshName(const char* filename, AssetName* name);
AddMeshResult AddMesh(AssetManager* manager, const char* filename, MeshFileFormat format);

// TODO: Clean this thing with slots and pointers up
// Many of callers wants to know filename or name
// so it should be available via GetMesh call
Mesh* GetMesh(AssetManager* manager, u32 id);
AssetSlot<Mesh*>* GetMeshSlot(AssetManager* manager, u32 id);

Material* Get(AssetManager* manager, EntityMaterial id);

void CompletePendingLoads(AssetManager* manager);

struct OpenMeshResult {
    enum Result {UnknownError = 0, Ok, FileNameIsTooLong, FileNotFound, ReadFileError, InvalidFileFormat } status;
    void* file;
    u32 fileSize;
};

const char* ToString(OpenMeshResult::Result value);

OpenMeshResult OpenMeshFileFlux(const char* filename);
OpenMeshResult OpenMeshFileAAB(const char* filename);
void CloseMeshFile(void* file);
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
