#pragma once

struct Renderer;

enum struct TextureFilter {
    None, Bilinear, Trilinear, Anisotropic
};

struct Texture {
    GLuint gpuHandle;
    TextureFilter filter;
    GLenum format;
    GLenum wrapMode;
    u32 width;
    u32 height;
    void* data;
};

struct Mesh {
    void* base;
    u32 vertexCount;
    u32 indexCount;
    v3* vertices;
    v3* normals;
    v2* uvs;
    v3* tangents;
    u32* indices;
    u32 gpuVertexBufferHandle;
    u32 gpuIndexBufferHandle;
};


struct Material {
    enum struct Type { Legacy = 0, PBR } type;
    enum struct Workflow { Legacy = 0, Metallic, Specular, Custom } workflow;
    union {
        struct {
            Texture diffMap;
            Texture specMap;
        } legacy;
        struct {
            union {
                struct {
                    v3 albedo;
                    f32 roughness;
                    f32 metalness;
                } custom;
                struct {
                    Texture albedo;
                    Texture roughness;
                    Texture metalness;
                    Texture normals;
                } metallic;
                struct {
                    Texture albedo;
                    Texture specular;
                    Texture gloss;
                    Texture normals;
                } specular;
            };
        } pbr;
    };
};

struct CubeTexture {
    struct Img  {
        GLenum format;
        u32 width;
        u32 height;
        void* data;
    };

    GLuint gpuHandle;
    b32 useMips;
    TextureFilter filter;

    union {
        struct {
            Img right;
            Img left;
            Img up;
            Img down;
            Img front;
            Img back;
        };
        Img images[6];
    };
};

Renderer* InitializeRenderer(uv2 renderRes);
Mesh LoadMeshAAB(const wchar_t* filepath);
Mesh LoadMeshObj(const char* filepath);

Material LoadMaterialPBRMetallic(const char* albedoPath, const char* roughnessPath, const char* metalnessPath, const char* normalsPath);
Material LoadMaterialLegacy(const char* diffusePath,  const char* specularPath = 0);
CubeTexture LoadCubemap(const char* back, const char* down, const char* front, const char* left, const char* right, const char* up);
CubeTexture LoadCubemapHDR(const char* back, const char* down, const char* front, const char* left, const char* right, const char* up);
CubeTexture MakeEmptyCubemap(int w, int h, GLenum format, TextureFilter filter = TextureFilter::Bilinear, bool useMips = false);
void GenIrradanceMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle);
void GenEnvPrefiliteredMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle, u32 mipLevels);

struct RenderGroup;

void Begin(Renderer* renderer, RenderGroup* group);
void ShadowPass(Renderer* renderer, RenderGroup* group);
void MainPass(Renderer* renderer, RenderGroup* group);
void End(Renderer* renderer);



void RecompileShaders(Renderer* renderer);
