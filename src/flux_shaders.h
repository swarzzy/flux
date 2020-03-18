#pragma once

struct Renderer;

struct ShaderProgramSource  {
    const char* vert;
    const char* frag;
};

#include "flux_shaders_generated.h"

constexpr u32 ShaderCount = sizeof(Shaders) / sizeof(GLuint);
static_assert(ShaderCount == array_count(ShaderSources));

GLuint CompileGLSL(const char* name, const char* vert, const char* frag);
void RecompileShaders(Renderer* renderer);
inline void DeleteProgram(GLuint handle) { glDeleteProgram(handle); }

template <typename T, u32 Binding>
struct UniformBuffer {
        GLuint handle;
};

template<typename T, u32 Binding>
void ReallocUniformBuffer(UniformBuffer<T, Binding>* buffer);

template<typename T, u32 Binding>
T* Map(UniformBuffer<T, Binding> buffer);

template<typename T, u32 Binding>
void Unmap(UniformBuffer<T, Binding> buffer);

struct WaterShader {
    static constexpr u32 Position = 0;
    static constexpr u32 Normal = 1;
    static constexpr u32 UV = 2;
};

struct MeshShader {
    static constexpr u32 DiffMap = 0;
    static constexpr u32 SpecMap = 1;
    static constexpr u32 ShadowMap = 2;
};

struct MeshPhongCustomShader {
    static constexpr u32 ShadowMap = 0;
};

struct MeshPBRShader {
    static constexpr u32 IrradanceMap = 0;
    static constexpr u32 EnviromentMap = 1;
    static constexpr u32 BRDFLut = 2;
    static constexpr u32 AlbedoMap = 3;
    static constexpr u32 NormalMap = 4;
    static constexpr u32 RoughnessMap = 5;
    static constexpr u32 MetallicMap = 6;
    static constexpr u32 SpecularMap = 7;
    static constexpr u32 GlossMap = 8;
    static constexpr u32 ShadowMap = 9;
};

struct ShadowPassShader {
    static constexpr u32 CascadeIndexLocation = 0;
    static constexpr u32 PositionAttribLocation = 0;
    static constexpr u32 NormalAttribLocation = 1;
};

struct SkyboxShader {
    static constexpr u32 CubeTexture = 0;
};

struct PostFxShader {
    static constexpr u32 ColorSourceLinear = 0;
};

struct FXAAShader {
    static constexpr u32 ColorSourcePerceptual = 0;
};

struct EnvMapPrefilterShader {
    static constexpr u32 SourceCubemap = 0;
    static constexpr u32 Roughness = 0;
    static constexpr u32 Resolution = 1;
};

struct IrradanceConvolver {
    static constexpr u32 SourceCubemap = 0;
};

struct layout_std140 ShaderFrameData {
    static constexpr u32 Binding = 0;
    struct layout_std140 DirLight {
        std140_vec3 pos;
        std140_vec3 dir;
        std140_vec3 ambient;
        std140_vec3 diffuse;
        std140_vec3 specular;
    };

    std140_mat4 viewProjMatrix;
    std140_mat4 viewMatrix;
    std140_mat4 projectionMatrix;
    std140_mat4 invViewMatrix;
    std140_mat4 invProjMatrix;
    std140_mat4 lightSpaceMatrices[3];
    DirLight dirLight;
    std140_vec3 viewPos;
    std140_vec3 shadowCascadeSplits;
    std140_int showShadowCascadeBoundaries;
    std140_float shadowFilterSampleScale;
    std140_int debugF;
    std140_int debugG;
    std140_int debugD;
    std140_int debugNormals;
    std140_float constShadowBias;
    std140_float gamma;
    std140_float exposure;
    std140_vec2 screenSize;
};

struct layout_std140 ShaderMeshData {
    static constexpr u32 Binding = 1;
    std140_mat4 modelMatrix;
    std140_mat3 normalMatrix;
    std140_vec3 lineColor;
    std140_int metallicWorkflow;
    std140_int customMaterial;
    std140_vec3 customAlbedo;
    std140_float customRoughness;
    std140_float customMetalness;
    std140_vec3 customPhongDiffuse;
    std140_vec3 customPhongSpecular;
};

struct layout_std140 ChunkFragUniformBuffer {
    struct layout_std140 DirLight {
        std140_vec3 dir;
        std140_vec3 ambient;
        std140_vec3 diffuse;
        std140_vec3 specular;
    };

    DirLight u_DirLight;
    std140_vec3 u_ViewPos;
    std140_vec3 u_ShadowCascadeSplits;
    std140_int u_ShowShadowCascadesBoundaries;
    std140_float shadowFilterSampleScale;
};

struct layout_std140 ChunkCameraBuffer {
    std140_mat4 viewMatrix;
    std140_mat4 projectionMatrix;
    std140_mat4_array lightSpaceMatrix[3];
};

struct layout_std140 ChunkMeshBuffer {
    std140_mat4 modelMatrix;
    std140_mat3 normalMatrix;
};
