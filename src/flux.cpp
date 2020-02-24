#include "flux.h"
#include "flux_platform.h"
#include "flux_debug_overlay.h"

void FluxInit(Context* context) {
    LoadedMesh mesh = {};
    context->wheelMesh = LoadMeshObj("../res/meshes/wheel.obj");
    context->sphereMesh = LoadMeshAAB(L"../res/meshes/sphere.aab");
    context->plateMesh = LoadMeshAAB(L"../res/meshes/plate.aab");
    context->checkerboardMaterial = LoadMaterialLegacy("../res/checkerboard.jpg");
    context->oldMetalMaterial = LoadMaterialPBRMetallic("../res/materials/oldmetal/greasy-metal-pan1-albedo.png", "../res/materials/oldmetal/greasy-metal-pan1-roughness.png", "../res/materials/oldmetal/greasy-metal-pan1-metal.png", "../res/materials/oldmetal/greasy-metal-pan1-normal.png");
    context->skybox = LoadCubemap("../res/skybox/sky_back.png", "../res/skybox/sky_down.png", "../res/skybox/sky_front.png", "../res/skybox/sky_left.png", "../res/skybox/sky_right.png", "../res/skybox/sky_up.png");

    stbi_set_flip_vertically_on_load(0);
    defer { stbi_set_flip_vertically_on_load(1); };

    context->hdrMap = LoadCubemapHDR("../res/desert_sky/nz.hdr", "../res/desert_sky/ny.hdr", "../res/desert_sky/pz.hdr", "../res/desert_sky/nx.hdr", "../res/desert_sky/px.hdr", "../res/desert_sky/py.hdr");
    context->irradanceMap = MakeEmptyCubemap(64, 64, GL_RGB16F);
    context->enviromentMap = MakeEmptyCubemap(256, 256, GL_RGB16F, TextureFilter::Trilinear, true);

    context->renderGroup.drawSkybox = true;
    context->renderGroup.skyboxHandle = context->enviromentMap.gpuHandle;
    context->renderGroup.irradanceMapHandle = context->irradanceMap.gpuHandle;
    context->renderGroup.envMapHandle = context->enviromentMap.gpuHandle;

    GenIrradanceMap(context->renderer, &context->irradanceMap, context->hdrMap.gpuHandle);
    GenEnvPrefiliteredMap(context->renderer, &context->enviromentMap, context->hdrMap.gpuHandle, 6);
}

void FluxReload(Context* context) {

}

void FluxUpdate(Context* context) {
    Update(&context->camera, GlobalAbsDeltaTime);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawDebugPerformanceCounters();

    auto renderer = context->renderer;
    auto group = &context->renderGroup;

    group->camera = &context->camera;

    DEBUG_OVERLAY_TRACE(group->camera->position);
    DEBUG_OVERLAY_TRACE(group->camera->front);

    DirectionalLight light = {};
    light.dir = Normalize(V3(0.0f, -1.0f, -1.0f));
    light.from = V3(0.0f, 5.0f, 5.0f);
    light.ambient = V3(0.3f);
    light.diffuse = V3(0.8f);
    light.specular = V3(1.0f);
    RenderCommandSetDirLight lightCommand = { light };
    Push(group, &lightCommand);

    RenderCommandDrawMesh command = {};
    command.transform = M4x4(1.0f);
    command.mesh = &context->wheelMesh;
    command.material = context->checkerboardMaterial;
    Push(group, &command);

    RenderCommandDrawMesh plateCommand = {};
    plateCommand.transform = M4x4(1.0f);
    plateCommand.mesh = &context->plateMesh;
    plateCommand.material = context->checkerboardMaterial;
    Push(group, &plateCommand);

    Begin(renderer, group);
    ShadowPass(renderer, group);
    MainPass(renderer, group);
    End(renderer);
}

void FluxRender(Context* context) {}
