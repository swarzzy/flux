#pragma once
#include "flux_camera.h"
#include "flux_renderer.h"
#include "flux_render_group.h"
#include "flux_world.h"
#include "flux_ui.h"
#include "flux_resource_manager.h"
#include "flux_console.h"

struct Context {
    Logger logger;
    Console console;
    MemoryArena* tempArena;
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
    b32 showConsole;
};

void FluxInit(Context* context);
void FluxReload(Context* context);
void FluxUpdate(Context* context);
void FluxRender(Context* context);
