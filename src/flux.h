#pragma once
#include "flux_camera.h"
#include "flux_renderer.h"
#include "flux_render_group.h"

struct Context {
    GLuint prog;
    GLuint vbo;
    Camera camera;
    Renderer* renderer;
    RenderGroup renderGroup;
    Mesh sphereMesh;
    Mesh plateMesh;
    Mesh* wheelMesh;
    Material oldMetalMaterial;
    Material checkerboardMaterial;
    Material backpackMaterial;
    CubeTexture skybox;
    CubeTexture hdrMap;
    CubeTexture irradanceMap;
    CubeTexture enviromentMap;
};

void FluxInit(Context* context);
void FluxReload(Context* context);
void FluxUpdate(Context* context);
void FluxRender(Context* context);
