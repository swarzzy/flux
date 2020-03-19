#pragma once

#pragma pack(push, 1)

// TODO: This is totally memory wasteland
// We should need store it more efficient
struct StoredTexture {
    char filename[MaxAssetPathSize];
    u32 format;
    u32 wrapMode;
    u32 filter;
    u32 range;
};

struct StoredMaterial {
    u32 workflow;
    union {
        struct {
            StoredTexture diffuse;
            StoredTexture specular;
        } phong;
        struct {
            StoredTexture albedo;
            StoredTexture roughness;
            StoredTexture metallic;
            StoredTexture normals;
            u32 normalsFormat;
        } pbrMetallic;
        struct {
            StoredTexture albedo;
            StoredTexture specular;
            StoredTexture gloss;
            StoredTexture normals;
            u32 normalsFormat;
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

struct StoredEntity {
    u32 id;
    v3 p;
    v3 scale;
    StoredMaterial material;
    u32 meshFileFormat;
    char meshFileName[MaxAssetPathSize];
};

struct WorldFile {
    u32 nextEntitySerialNumber;
    u32 entityCount;
    u32 firstEntityOffset;
    char name[WorldNameSize];
};
#pragma pack(pop)
