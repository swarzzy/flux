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

const char* ToString(TextureFilter value) {
    switch (value) {
    case TextureFilter::None: { return "none"; } break;
    case TextureFilter::Bilinear: { return "bilinear"; } break;
    case TextureFilter::Trilinear: { return "trilinear"; } break;
    case TextureFilter::Anisotropic: { return "anisotropic"; } break;
        invalid_default();
    }
    return "";
}

enum struct TextureFormat : u32 {
    Unknown, SRGBA8, SRGB8, RGBA8, RGB8, RGB16F, RG16F, R8, RG8
};

const char* ToString(TextureFormat value) {
    switch (value) {
    case TextureFormat::Unknown: { return "unknown"; } break;
    case TextureFormat::SRGBA8: { return "SRGBA8"; } break;
    case TextureFormat::SRGB8: { return "SRGB8"; } break;
    case TextureFormat::RGBA8: { return "RGBA8"; } break;
    case TextureFormat::RGB8: { return "RGB8"; } break;
    case TextureFormat::RGB16F: { return "RGB16F"; } break;
    case TextureFormat::RG16F: { return "RG16F"; } break;
    case TextureFormat::R8: { return "R8"; } break;
    case TextureFormat::RG8: { return "RG8"; } break;
        invalid_default();
    }
    return "";
}

enum struct TextureWrapMode : u32 {
    Repeat, ClampToEdge, Default = Repeat
};

const char* ToString(TextureWrapMode value) {
    switch (value) {
    case TextureWrapMode::Repeat: { return "repeat"; } break;
    case TextureWrapMode::ClampToEdge: { return "clamp to edge"; } break;
        invalid_default();
    }
    return "";
}

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
    TextureFilter filter;
    TextureFormat format;
    TextureWrapMode wrapMode;
    u32 width;
    u32 height;
    b32 useMips;
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

#if 0
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
#endif

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

template <typename T>
struct AssetSlot {
    volatile AssetState state;
    u32 id;
    T asset;
    char filename[MaxAssetPathSize];
    char name[MaxAssetNameSize];
    MeshFileFormat format;
};

struct MeshSlot {
    volatile AssetState state;
    u32 id;
    Mesh* mesh;
    char filename[MaxAssetPathSize];
    char name[MaxAssetNameSize];
    MeshFileFormat format;
};

struct TextureSlot {
    volatile AssetState state;
    u32 id;
    Texture texture;
    char filename[MaxAssetPathSize];
    char name[MaxAssetNameSize];
    TextureFormat format = TextureFormat::Unknown;
    TextureWrapMode wrapMode = TextureWrapMode::Default;
    TextureFilter filter = TextureFilter::Default;
    DynamicRange range = DynamicRange::LDR;
};

enum struct AssetType {
    Mesh, Texture
};

struct AssetQueueEntry {
    AssetType type;
    u32 id;
};

struct AssetManager {
    static u32 Hasher(u32* key) { return *key; }
    static bool Comparator(u32* a, u32* b) { return *a == *b; }
    AssetNameTable nameTable;
    HashMap<u32, MeshSlot> meshTable = HashMap<u32, MeshSlot>::Make(Hasher, Comparator);
    HashMap<u32, TextureSlot> textureTable = HashMap<u32, TextureSlot>::Make(Hasher, Comparator);
    u32 assetQueueAt;
    AssetQueueEntry assetQueue[32];
};

struct AddAssetResult {
    enum {UnknownError = 0, Ok, AlreadyExists} status;
    u32 id;

    u32 Unwrap() { if (status == Ok) { return id; } else { panic(false); return 0; }};
};


void GetAssetName(const char* filename, AssetName* name);
AddAssetResult AddMesh(AssetManager* manager, const char* filename, MeshFileFormat format);
AddAssetResult AddTexture(AssetManager* manager, const char* filename, TextureFormat format = TextureFormat::Unknown, TextureWrapMode wrapMode = TextureWrapMode::Default, TextureFilter filter = TextureFilter::Default, DynamicRange range = DynamicRange::LDR);

// TODO: Clean this thing with slots and pointers up
// Many of callers wants to know filename or name
// so it should be available via GetMesh call
Mesh* GetMesh(AssetManager* manager, u32 id);
MeshSlot* GetMeshSlot(AssetManager* manager, u32 id);

Texture* GetTexture(AssetManager* manager, u32 id);
TextureSlot* GetTextureSlot(AssetManager* manager, u32 id);

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

Texture LoadTextureFromFile(const char* filename, TextureFormat format = TextureFormat::Unknown, TextureWrapMode wrapMode = TextureWrapMode::Default, TextureFilter filter = TextureFilter::Default, DynamicRange range = DynamicRange::LDR);
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

inline AddAssetResult AddAlbedoMap(AssetManager* manager, const char* filename) {
    return AddTexture(manager, filename, TextureFormat::SRGB8, TextureWrapMode::Default, TextureFilter::Trilinear, DynamicRange::LDR);
}

inline AddAssetResult AddRoughnessMap(AssetManager* manager, const char* filename) {
    return AddTexture(manager, filename, TextureFormat::R8, TextureWrapMode::Default, TextureFilter::Trilinear, DynamicRange::LDR);
}

inline AddAssetResult AddMetallicMap(AssetManager* manager, const char* filename) {
    return AddTexture(manager, filename, TextureFormat::R8, TextureWrapMode::Default, TextureFilter::Trilinear, DynamicRange::LDR);
}

inline AddAssetResult AddNormalMap(AssetManager* manager, const char* filename) {
    return AddTexture(manager, filename, TextureFormat::RGB8, TextureWrapMode::Default, TextureFilter::Trilinear, DynamicRange::LDR);
}

inline AddAssetResult AddPhongTexture(AssetManager* manager, const char* filename) {
    return AddTexture(manager, filename, TextureFormat::SRGB8, TextureWrapMode::Default, TextureFilter::Trilinear, DynamicRange::LDR);
}
