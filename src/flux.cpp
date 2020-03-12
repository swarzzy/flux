#include "flux.h"
#include "flux_platform.h"
#include "flux_debug_overlay.h"
#include "flux_resource_manager.h"

void Work(void* data, u32 id) {
    //while (true) {
        printf("Thread %d does some work\n", (int)id);
        //}
}


void FluxInit(Context* context) {
    context->world = (World*)PlatformAlloc(sizeof(World));
    *context->world = {};
    strcpy_s(context->world->name, array_count(context->world->name), "dummy_world");
    auto begin = PlatformGetTimeStamp();

    auto assetManager = &context->assetManager;

    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);
    PlatformPushWork(GlobalPlaformWorkQueue, nullptr, Work);

//#define ASYNC
#if defined (ASYNC)
    auto oldMetal = assetManager->materials + (u32)EntityMaterial::OldMetal;
    auto backpack = assetManager->materials + (u32)EntityMaterial::Backpack;

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
#endif
    auto end = PlatformGetTimeStamp();
    printf("[Info] Loading time: %f sec\n", end - begin);

    context->skybox = LoadCubemapLDR("../res/skybox/sky_back.png", "../res/skybox/sky_down.png", "../res/skybox/sky_front.png", "../res/skybox/sky_left.png", "../res/skybox/sky_right.png", "../res/skybox/sky_up.png");
    UploadToGPU(&context->skybox);
    context->hdrMap = LoadCubemapHDR("../res/desert_sky/nz.hdr", "../res/desert_sky/ny.hdr", "../res/desert_sky/pz.hdr", "../res/desert_sky/nx.hdr", "../res/desert_sky/px.hdr", "../res/desert_sky/py.hdr");
    UploadToGPU(&context->hdrMap);
    context->irradanceMap = MakeEmptyCubemap(64, 64, TextureFormat::RGB16F, TextureFilter::Bilinear, false);
    UploadToGPU(&context->irradanceMap);
    context->enviromentMap = MakeEmptyCubemap(256, 256, TextureFormat::RGB16F, TextureFilter::Trilinear, true);
    UploadToGPU(&context->enviromentMap);

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
    auto assetManager = &context->assetManager;

    DEBUG_OVERLAY_TRACE(assetManager->assetQueueAt);
    CompletePendingLoads(assetManager);

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
        auto mesh = Get(assetManager, entity->mesh);
        if (mesh) {
            auto aabb = mesh->aabb;
            aabb.min = (entity->transform * V4(aabb.min, 1.0f)).xyz;
            aabb.max = (entity->transform * V4(aabb.max, 1.0f)).xyz;
            DrawAlignedBoxOutline(&context->renderGroup, aabb.min, aabb.max, V3(0.0f, 0.0f, 1.0f), 2.0f);
        }
    }

#if 0
    RenderCommandDrawMesh gizmosCommand = {};
    gizmosCommand.transform = Scale(V3(0.1f, 0.1f, 0.1f));
    gizmosCommand.mesh = context->meshes[(u32)EntityMesh::Gizmos];
    gizmosCommand.material = context->materials[(u32)EntityMaterial::Checkerboard];
    Push(group, &gizmosCommand);
#endif

    for (uint i = 0; i < array_count(context->world->entities); i++) {
        auto entity = context->world->entities + i;
        if (entity->id) {
            auto mesh = Get(assetManager, entity->mesh);
            auto material = Get(assetManager, entity->material);
            if (mesh && material) {
                RenderCommandDrawMesh command = {};
                command.transform = entity->transform;
                command.mesh = mesh;
                command.material = material;
                Push(group, &command);
                if (context->ui.showBoundingVolumes) {
                    auto aabb = mesh->aabb;
                    aabb.min = (entity->transform * V4(aabb.min, 1.0f)).xyz;
                    aabb.max = (entity->transform * V4(aabb.max, 1.0f)).xyz;
                    DrawAlignedBoxOutline(&context->renderGroup, aabb.min, aabb.max, V3(1.0f, 0.0f, 0.0f), 2.0f);
                }
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
