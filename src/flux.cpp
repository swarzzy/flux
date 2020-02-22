#include "flux.h"
#include "flux_platform.h"
#include "flux_intrinsics.cpp"
#include "flux_math.cpp"

void* PlatformCalloc(uptr num, uptr size) { void* ptr = PlatformAlloc(num * size); memset(ptr, 0, num * size); return ptr; }

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#define TINYOBJ_MALLOC PlatformAlloc
#define TINYOBJ_REALLOC PlatformRealloc
#define TINYOBJ_CALLOC PlatformCalloc
#define TINYOBJ_FREE PlatformFree

#include "../ext/tinyobj/tinyobj_loader_c.h"

void FluxInit(Context* context) {
    const char* vert = R"(
#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (location = 2) out vec3 ourColor;

layout (location = 0) uniform mat4 ProjectionMatrix;

void main()
{
    gl_Position = ProjectionMatrix * vec4(aPos, 1.0);
    ourColor = aColor;
})";
    const char* frag = R"(
#version 450
out vec4 FragColor;

layout (location = 2) in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0f);
})";

    auto vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vert, nullptr);
    glCompileShader(vs);
    auto fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &frag, nullptr);
    glCompileShader(fs);
    context->prog = glCreateProgram();
    glAttachShader(context->prog, vs);
    glAttachShader(context->prog, fs);
    glLinkProgram(context->prog);

    f32 vertices[] = {
        // positions         // colors
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  // bottom right
         0.0f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  // bottom left
         -0.5f, -0.5f, -0.9f,  0.0f, 0.0f, 1.0f   // top
    };

    glCreateBuffers(1, &context->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, context->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void FluxReload(Context* context) {
    printf("[Info] Game was hot-reloaded");
}

void FluxUpdate(Context* context) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, context->vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glUseProgram(context->prog);
    v3 eye = V3(0.2, 0.3, 0.5);
    auto view = LookAtGLRH(eye, V3(0.2f, 0.0f, -1.0f), V3(0.2f, 8.0f, 0.0f));
    auto projection = PerspectiveGLRH(0.6f, 10.0f, 80.0f, 16.0f / 9.0f);
    auto model = Translation(V3(-0.3, 0.0f, 0.0f));
    auto viewProj = projection * view * model;
    m4x4 t1 = {};
    t1._11 = 1.0f;
    t1._22 = 2.0f;
    t1._33 = 3.0f;
    t1._44 = 4.0f;
    v4 t2 = V4(5.0f, 6.0f, 7.0f, 1.0f);
    auto test = t1 * t2;
    glUniformMatrix4fv(0, 1, GL_FALSE, viewProj.data);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void FluxRender(Context* context) {}
