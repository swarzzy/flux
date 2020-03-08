#include "flux.h"
#include "flux_platform.h"
#include "flux_debug_overlay.h"
#include "flux_resource_manager.h"

void FluxInit(Context* context) {
    context->world = (World*)PlatformAlloc(sizeof(World));
    *context->world = {};
    strcpy_s(context->world->name, array_count(context->world->name), "dummy_world");
    auto begin = PlatformGetTimeStamp();

//#define ASYNC
#if defined (ASYNC)
    auto oldMetal = context->materials + (u32)EntityMaterial::OldMetal;
    auto backpack = context->materials + (u32)EntityMaterial::Backpack;

    MaterialSpec oldMetalspec = {};
    oldMetalspec.albedo = "../res/materials/oldmetal/greasy-metal-pan1-albedo.png";
    oldMetalspec.roughness = "../res/materials/oldmetal/greasy-metal-pan1-roughness.png";
    oldMetalspec.metallic = "../res/materials/oldmetal/greasy-metal-pan1-metal.png";
    oldMetalspec.normals = "../res/materials/oldmetal/greasy-metal-pan1-normal.png";
    oldMetalspec.result = oldMetal;

    MaterialSpec backpackSpec = {};
    backpackSpec.albedo = "../res/materials/backpack/albedo.png";
    backpackSpec.roughness = "../res/materials/backpack/rough.png";
    backpackSpec.metallic = "../res/materials/backpack/metallic.png";
    backpackSpec.normals = "../res/materials/backpack/normal.png";
    backpackSpec.result = backpack;

    PlatformPushWork(GlobalPlaformWorkQueue, &oldMetalspec, LoadPbrMaterialJob);
    PlatformPushWork(GlobalPlaformWorkQueue, &backpackSpec, LoadPbrMaterialJob);
    PlatformCompleteAllWork(GlobalPlaformWorkQueue);
    CompletePbrMaterialLoad(oldMetal);
    CompletePbrMaterialLoad(backpack);
#else
    context->materials[(u32)EntityMaterial::OldMetal] = LoadMaterialPBRMetallic("../res/materials/oldmetal/greasy-metal-pan1-albedo.png", "../res/materials/oldmetal/greasy-metal-pan1-roughness.png", "../res/materials/oldmetal/greasy-metal-pan1-metal.png", "../res/materials/oldmetal/greasy-metal-pan1-normal.png");
    context->materials[(u32)EntityMaterial::Backpack] = LoadMaterialPBRMetallic("../res/materials/backpack/albedo.png", "../res/materials/backpack/rough.png", "../res/materials/backpack/metallic.png", "../res/materials/backpack/normal.png");
#endif
    auto end = PlatformGetTimeStamp();
    printf("[Info] Loading time: %f sec\n", end - begin);
    //context->meshes[(u32)EntityMesh::Gizmos] = LoadMesh("../res/meshes/gizmos.obj");
    //context->meshes[(u32)EntityMesh::Backpack] = LoadMesh("../res/meshes/backpack_low.fbx");
    context->meshes[(u32)EntityMesh::Backpack] = LoadMeshFlux(L"../res/meshes/backpack_low.mesh");
    RendererLoadMesh(context->meshes[(u32)EntityMesh::Backpack]);
    context->meshes[(u32)EntityMesh::Sphere] = LoadMeshAAB(L"../res/meshes/sphere.aab");
    context->meshes[(u32)EntityMesh::Plate] = LoadMeshAAB(L"../res/meshes/plate.aab");
    context->materials[(u32)EntityMaterial::Checkerboard] = LoadMaterialLegacy("../res/checkerboard.jpg");
    context->skybox = LoadCubemap("../res/skybox/sky_back.png", "../res/skybox/sky_down.png", "../res/skybox/sky_front.png", "../res/skybox/sky_left.png", "../res/skybox/sky_right.png", "../res/skybox/sky_up.png");
    context->hdrMap = LoadCubemapHDR("../res/desert_sky/nz.hdr", "../res/desert_sky/ny.hdr", "../res/desert_sky/pz.hdr", "../res/desert_sky/nx.hdr", "../res/desert_sky/px.hdr", "../res/desert_sky/py.hdr");
    context->irradanceMap = MakeEmptyCubemap(64, 64, GL_RGB16F);
    context->enviromentMap = MakeEmptyCubemap(256, 256, GL_RGB16F, TextureFilter::Trilinear, true);

    context->renderGroup.drawSkybox = true;
    context->renderGroup.skyboxHandle = context->enviromentMap.gpuHandle;
    context->renderGroup.irradanceMapHandle = context->irradanceMap.gpuHandle;
    context->renderGroup.envMapHandle = context->enviromentMap.gpuHandle;

    GenIrradanceMap(context->renderer, &context->irradanceMap, context->hdrMap.gpuHandle);
    GenEnvPrefiliteredMap(context->renderer, &context->enviromentMap, context->hdrMap.gpuHandle, 6);

    auto checkerboardEntity = AddEntity(context->world);
    auto backpackEntity = AddEntity(context->world);
    auto sphereEntity = AddEntity(context->world);

    checkerboardEntity->mesh = EntityMesh::Plate;
    checkerboardEntity->material = EntityMaterial::Checkerboard;

    backpackEntity->p = V3(1.0f);
    backpackEntity->scale = V3(0.01f);
    backpackEntity->mesh = EntityMesh::Backpack;
    backpackEntity->material = EntityMaterial::Backpack;

    sphereEntity->mesh = EntityMesh::Sphere;
    sphereEntity->material = EntityMaterial::OldMetal;
}

void FluxReload(Context* context) {
}

void FluxUpdate(Context* context) {
    auto ui = &context->ui;
    auto world = context->world;
    auto renderer = context->renderer;

    auto renderRes = GetRenderResolution(renderer);
    if (renderRes.x != GlobalPlatform.windowWidth ||
        renderRes.y != GlobalPlatform.windowHeight) {
        ChangeRenderResolution(renderer, UV2(GlobalPlatform.windowWidth, GlobalPlatform.windowHeight));
    }


    Update(&context->camera, GlobalAbsDeltaTime);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawDebugPerformanceCounters();

    UpdateUi(context);

    if (ui->wantsLoadLoadFrom) {
        auto newWorld = LoadWorldFrom(ui);
        if (newWorld) {
            // TODO: Loading and unloading levels
            PlatformFree(context->world);
            ui->selectedEntity = 0;
            context->world = newWorld;
        }
    }

    if (ui->wantsSaveAs) {
        SaveWorldAs(ui, world);
    }

    if (ui->wantsAddEntity) {
        ui->wantsAddEntity = false;
        auto entity = AddEntity(world);
        entity->mesh = EntityMesh::Sphere;
        entity->material = EntityMaterial::Checkerboard;
    }

    // TODO: Factor this out
    if (ui->wantsSave || (KeyHeld(Key::Ctrl) && KeyPressed(Key::S))) {
        ui->wantsSave = false;
        if (world->name[0]) {
            wchar_t buffer[array_count(world->name)];
            mbstowcs(buffer, world->name, array_count(world->name));
            // Back-up previous save
            if (PlatformDebugGetFileSize(buffer)) {
                wchar_t backupBuffer[array_count(world->name) + sizeof(L".backup")];
                wcscpy(backupBuffer, buffer);
                wcscat(backupBuffer, L".backup");
                PlatformDebugCopyFile(buffer, backupBuffer, true);
            }
            if (SaveToDisk(world, buffer)) {
                printf("[Info] World %s was saved successfully\n", world->name);
            } else {
                printf("[Error] Failed to save world %s\n", world->name);
            }
        }
    }

    Update(world);

    auto group = &context->renderGroup;

    group->camera = &context->camera;
    auto camera = &context->camera;

    DirectionalLight light = {};
    light.dir = Normalize(V3(0.0f, -1.0f, -1.0f));
    light.from = V3(0.0f, 5.0f, 5.0f);
    light.ambient = V3(0.3f);
    light.diffuse = V3(0.8f);
    light.specular = V3(1.0f);
    RenderCommandSetDirLight lightCommand = { light };
    Push(group, &lightCommand);

    if (ui->selectedEntity) {
        auto entity = GetEntity(world, ui->selectedEntity);
        assert(entity);
        auto aabb = context->meshes[(u32)entity->mesh]->aabb;
        aabb.min = (entity->transform * V4(aabb.min, 1.0f)).xyz;
        aabb.max = (entity->transform * V4(aabb.max, 1.0f)).xyz;
        DrawAlignedBoxOutline(&context->renderGroup, aabb.min, aabb.max, V3(0.0f, 0.0f, 1.0f), 2.0f);
    }

    RenderCommandDrawMesh gizmosCommand = {};
    gizmosCommand.transform = Scale(V3(0.1f, 0.1f, 0.1f));
    gizmosCommand.mesh = context->meshes[(u32)EntityMesh::Gizmos];
    gizmosCommand.material = context->materials[(u32)EntityMaterial::Checkerboard];
    Push(group, &gizmosCommand);

    for (uint i = 0; i < array_count(context->world->entities); i++) {
        auto entity = context->world->entities + i;
        if (entity->id) {
            RenderCommandDrawMesh command = {};
            command.transform = entity->transform;
            command.mesh = context->meshes[(u32)entity->mesh];
            command.material = context->materials[(u32)entity->material];
            Push(group, &command);
            if (context->ui.showBoundingVolumes) {
                auto aabb = context->meshes[(u32)entity->mesh]->aabb;
                aabb.min = (entity->transform * V4(aabb.min, 1.0f)).xyz;
                aabb.max = (entity->transform * V4(aabb.max, 1.0f)).xyz;
                DrawAlignedBoxOutline(&context->renderGroup, aabb.min, aabb.max, V3(1.0f, 0.0f, 0.0f), 2.0f);
            }
        }
    }

    Begin(renderer, group);
    ShadowPass(renderer, group);
    MainPass(renderer, group);
    End(renderer);

    // Alpha
    //ImGui::PopStyleVar();
}

void FluxRender(Context* context) {}
