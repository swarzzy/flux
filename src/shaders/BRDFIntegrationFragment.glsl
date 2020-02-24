#version 450

layout (location = 0) in vec2 UV;

out vec4 ResultColor;

const uint SAMPLE_COUNT = 1024u;
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

float GeometrySchlickGGX(float NdotV, float a)
{
    float k = (a * a) / 1.0f;
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float a)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, a);
    float ggx1 = GeometrySchlickGGX(NdotL, a);
    return ggx1 * ggx2;
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0f - NdotV * NdotV);
    V.y = 0.0f;
    V.z = NdotV;

    float A = 0.0f;
    float B = 0.0f;

    vec3 N = vec3(0.0f, 0.0f, 1.0f);

    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if (NdotL > 0.0f)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    return vec2(A, B);
}

void main()
{
    ResultColor = vec4(IntegrateBRDF(UV.x, UV.y), 0.0f, 1.0f);
}