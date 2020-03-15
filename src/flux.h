#pragma once
#include "flux_camera.h"
#include "flux_renderer.h"
#include "flux_render_group.h"
#include "flux_world.h"
#include "flux_ui.h"
#include "flux_resource_manager.h"

struct Context {
    World* world;
    Ui ui;
    AssetManager assetManager;
    GLuint prog;
    GLuint vbo;
    Camera camera;
    Renderer* renderer;
    RenderGroup renderGroup;
    CubeTexture skybox;
    CubeTexture hdrMap;
    CubeTexture irradanceMap;
    CubeTexture enviromentMap;
};

void FluxInit(Context* context);
void FluxReload(Context* context);
void FluxUpdate(Context* context);
void FluxRender(Context* context);
