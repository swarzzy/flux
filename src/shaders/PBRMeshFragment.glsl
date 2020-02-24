#version 450

#include Common.glh
#include Pbr.glh
#include ShadowsCommon.glh

out vec4 resultColor;

layout (location = 4) in VertOut
{
    vec3 fragPos;
    vec3 normal;
    vec2 uv;
    mat3 tbn;
    vec3 viewPosition;
    vec4 lightSpacePos[3];
} fragIn;

layout (binding = 0) uniform samplerCube IrradanceMap;
layout (binding = 1) uniform samplerCube EnviromentMap;
layout (binding = 2) uniform sampler2D BRDFLut;

layout (binding = 3) uniform sampler2D AlbedoMap;
layout (binding = 4) uniform sampler2D NormalMap;

layout (binding = 5) uniform sampler2D RoughnessMap;
layout (binding = 6) uniform sampler2D MetalnessMap;

layout (binding = 7) uniform sampler2D SpecularMap;
layout (binding = 8) uniform sampler2D GlossMap;


layout (binding = 9) uniform sampler2DArrayShadow ShadowMap;
//uniform sampler2D uAOMap;

void main()
{
    vec3 N;

    vec3 V = normalize(FrameData.viewPos - fragIn.fragPos);

    PBR context;

    if (MeshData.customMaterial == 1)
    {
        N = normalize(fragIn.normal);
        vec3 albedo = MeshData.customAlbedo;
        float roughness = MeshData.customRoughness;
        float metalness = MeshData.customMetalness;
        context = InitPBRMetallic(V, N, albedo, metalness, roughness);
    }
    else if (MeshData.metallicWorkflow == 1)
    {
         vec3 n = texture(NormalMap, fragIn.uv).xyz * 2.0f - 1.0f;
         N = normalize(n);
         // NOTE: Flipping y because engine uses LH normal maps (UE4) but OpenGL does it's job in RH space
         N.y = -N.y;
         N = normalize(fragIn.tbn * N);
         vec3 albedo = texture(AlbedoMap, fragIn.uv).xyz;
         float roughness = texture(RoughnessMap, fragIn.uv).r;
         float metalness = texture(MetalnessMap, fragIn.uv).r;
         context = InitPBRMetallic(V, N, albedo, metalness, roughness);
    }
    else
    {
         vec3 n = texture(NormalMap, fragIn.uv).xyz * 2.0f - 1.0f;
         N = normalize(n);
         // NOTE: Flipping y because engine uses LH normal maps (UE4) but OpenGL does it's job in RH space
         N.y = -N.y;
         N = normalize(fragIn.tbn * N);
         vec3 albedo = texture(AlbedoMap, fragIn.uv).xyz;
         vec3 specular = texture(SpecularMap, fragIn.uv).rgb;
         float gloss = texture(GlossMap, fragIn.uv).r;
         context = InitPBRSpecular(V, N, albedo, specular, gloss);
    }

    vec3 L0 = vec3(0.0f);

    vec3 L = normalize(-FrameData.dirLight.dir);
    vec3 H = normalize(V + L);

    vec3 irradance = FrameData.dirLight.diffuse;
    L0 += Unreal4BRDF(context, L) * irradance;

    vec3 envIrradance = IBLIrradance(context, IrradanceMap, EnviromentMap, BRDFLut);

    vec3 kShadow = CalcShadow(fragIn.viewPosition, FrameData.shadowCascadeSplits, fragIn.lightSpacePos, ShadowMap, FrameData.shadowFilterSampleScale, FrameData.showShadowCascadeBoundaries);

    resultColor = vec4((envIrradance + L0 * kShadow), 1.0f);
#if 0
    if (FrameData.debugF == 1) resultColor = vec4(F,  1.0f);
    else if (FrameData.debugG == 1) resultColor = vec4(G, G, G, 1.0f);
    else if (FrameData.debugD == 1) resultColor = vec4(D, D, D, 1.0f);
    else if (FrameData.debugNormals == 1) resultColor = vec4(N, 1.0f);
#endif
}