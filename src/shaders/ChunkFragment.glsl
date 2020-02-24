#version 450

#include Common.glh
#include ShadowsCommon.glh

layout (location = 3) in vec3 v_ViewPosition;
layout (location = 4) in vec4 v_LightSpacePos[3];
layout (location = 7) flat in int v_TileId;
layout (location = 8) in vec3 v_Normal;
layout (location = 9) in vec2 v_UV;
layout (location = 10) in vec4 v_Position;

out vec4 color;

layout (binding = 0) uniform sampler2DArray u_TerrainAtlas;
layout (binding = 1) uniform sampler2DArrayShadow u_ShadowMap;
layout (binding = 2) uniform sampler1D randomTexture;

#define PI (3.14159265359)

//#define RANDOM_DISC_PCF 1
#define RANDOMIZE_OFFSETS 1
#define DUMMY_PCF 1


vec3 CalcDirectionalLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffSample)
{
    vec3 lightDir = normalize(-light.dir);
    vec3 lightDirReflected = reflect(-lightDir, normal);

    float Kd = max(dot(normal, lightDir), 0.0);

    float viewSpaceDepth = -v_ViewPosition.z;
    int cascadeIndex = GetShadowCascadeIndex(-v_ViewPosition.z, FrameData.shadowCascadeSplits);
    vec3 lightSpaceP = v_LightSpacePos[cascadeIndex].xyz / v_LightSpacePos[cascadeIndex].w;
    lightSpaceP = lightSpaceP * 0.5f + 0.5f;
#if DUMMY_PCF
   vec3 Kshadow = ShadowPCF(u_ShadowMap, cascadeIndex, lightSpaceP, viewSpaceDepth, FrameData.shadowFilterSampleScale, FrameData.showShadowCascadeBoundaries);
#endif
#if RANDOM_DISC_PCF
   vec3 Kshadow = ShadowRandomDisc(u_ShadowMap, randomTexture, cascadeIndex, lightSpaceP, viewSpaceDepth, FrameData.shadowFilterSampleScale, FrameData.showShadowCascadeBoundaries);
#endif

    vec3 ambient = light.ambient * diffSample;
    vec3 diffuse = Kd * light.diffuse * diffSample * Kshadow;
    return ambient + diffuse;
}

#define TERRAIN_TEX_ARRAY_NUM_LAYERS 32

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(FrameData.viewPos - v_Position.xyz);

    int tileID = clamp(v_TileId, 0, TERRAIN_TEX_ARRAY_NUM_LAYERS);

    vec3 diffSample;
    float alpha;
    diffSample = texture(u_TerrainAtlas, vec3(v_UV.x, v_UV.y, tileID)).rgb;
    alpha = 1.0f;

    vec3 directional = CalcDirectionalLight(FrameData.dirLight, normal, viewDir, diffSample);
    //directional = diffSample;

    color = vec4(directional, alpha);
}