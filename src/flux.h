#pragma once
#include "flux_camera.h"
#include "flux_renderer.h"
#include "flux_render_group.h"
#include "flux_world.h"
#include "flux_ui.h"

struct Context {
    World* world;
    Ui ui;
    GLuint prog;
    GLuint vbo;
    Camera camera;
    Renderer* renderer;
    RenderGroup renderGroup;
    Mesh* meshes[EntityMesh::_Count];
    Material materials[EntityMaterial::_Count];
    CubeTexture skybox;
    CubeTexture hdrMap;
    CubeTexture irradanceMap;
    CubeTexture enviromentMap;
};

void FluxInit(Context* context);
void FluxReload(Context* context);
void FluxUpdate(Context* context);
void FluxRender(Context* context);
