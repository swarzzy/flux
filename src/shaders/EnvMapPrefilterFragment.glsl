#version 450

layout (location = 0) in vec3 UV;
out vec4 resultColor;

layout (binding = 0) uniform samplerCube uSourceCubemap;
layout (location = 0) uniform float uRoughness;
layout (location = 1) uniform int uResolution;

const float PI_32 = 3.14159265358979323846f;

// NOTE: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 p, vec3 N, float a)
{
    // NOTE: Epic Games GGX
    float aSq = a * a;
    float phi = 2.0f * PI_32 * p.x;
    float cosTheta = sqrt((1.0f - p.y) / (1.0f + (aSq * aSq - 1.0f) * p.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // NOTE: To cartesian
    vec3 d;
    d.x = cos(phi) * sinTheta;
    d.y = sin(phi) * sinTheta;
    d.z = cosTheta;

    // NOTE: From tangent space to world
    vec3 up = abs(N.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f); // ?????
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 sampleVec = right * d.x + up * d.y + N * d.z;
    return normalize(sampleVec);
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a4 = a * a * a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotHSq = NdotH * NdotH;

    float num = a4;
    float denom = (NdotHSq * (a4 - 1.0f) + 1.0f);
    denom = PI_32 * denom * denom;

    return num / max(denom, 0.001f);
}

const uint SAMPLES = 4096u;

void main()
{
    vec3 N = normalize(UV);
    vec3 R = N;
    vec3 V = R;

    float totalWeight = 0.0f;
    vec3 prefColor = vec3(0.0f);

    for (uint i = 0u; i < SAMPLES; i++)
    {
        vec2 p = Hammersley(i, SAMPLES);
        vec3 H = ImportanceSampleGGX(p, N, uRoughness);
        vec3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0f);
        if (NdotL > 0.0f)
        {
            float D = DistributionGGX(N, H, uRoughness);
            float NdotH = max(dot(N, H), 0.0f);
            float HdotV = max(dot(H, V), 0.0f);
            float PDF = D * NdotH / (4.0f * HdotV) + 0.0001;
            float saTexel = 4.0f * PI_32 / (6.0f * uResolution * uResolution);
            float saSample = 1.0f / (float(SAMPLES) * PDF + 0.0001f);
            float mipLevel = uRoughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);
            prefColor += textureLod(uSourceCubemap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefColor = prefColor / totalWeight;

    resultColor = vec4(prefColor, 1.0f);
}