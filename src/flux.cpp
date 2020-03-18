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
    AssetManager::Init(&context->assetManager, context->renderer);
    context->world = (World*)PlatformAlloc(sizeof(World));
    *context->world = {};

    auto world = context->world;

    strcpy_s(context->world->name, array_count(context->world->name), "dummy_world");

    auto assetManager = &context->assetManager;

    AddMesh(assetManager, "../res/meshes/plate.aab", MeshFileFormat::AAB);
    AddMesh(assetManager, "../res/meshes/sphere.aab", MeshFileFormat::AAB);
    AddMesh(assetManager, "../res/meshes/backpack_low.mesh", MeshFileFormat::Flux);

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

    auto checkerboardEntityID = AddEntity(context->world)->id;
    auto backpackEntityID = AddEntity(context->world)->id;
    auto sphereEntityID = AddEntity(context->world)->id;

    u32 oldMetalAlbedoId = AddAlbedoMap(assetManager, "../res/materials/oldmetal/greasy-metal-pan1-albedo.png").Unwrap();
    u32 oldMetalRoughId = AddRoughnessMap(assetManager, "../res/materials/oldmetal/greasy-metal-pan1-roughness.png").Unwrap();
    u32 oldMetalMetallicId = AddMetallicMap(assetManager, "../res/materials/oldmetal/greasy-metal-pan1-metal.png").Unwrap();
    u32 oldMetalNormalId = AddNormalMap(assetManager, "../res/materials/oldmetal/greasy-metal-pan1-normal.png").Unwrap();

    u32 backpackAlbedoId = AddAlbedoMap(assetManager, "../res/materials/backpack/albedo.png").Unwrap();
    u32 backpackRoughId = AddRoughnessMap(assetManager, "../res/materials/backpack/rough.png").Unwrap();
    u32 backpackMetallicId = AddMetallicMap(assetManager, "../res/materials/backpack/metallic.png").Unwrap();
    u32 backpackNormalId = AddNormalMap(assetManager, "../res/materials/backpack/normal.png").Unwrap();

    u32 checkerboardID = AddPhongTexture(assetManager, "../res/checkerboard.jpg").Unwrap();

    Material oldMetal = {};
    oldMetal.workflow = Material::PBRMetallic;
    oldMetal.pbrMetallic.albedo = oldMetalAlbedoId;
    oldMetal.pbrMetallic.roughness = oldMetalRoughId;
    oldMetal.pbrMetallic.metallic = oldMetalMetallicId;
    oldMetal.pbrMetallic.normals = oldMetalNormalId;

    Material backpack = {};
    backpack.workflow = Material::PBRMetallic;
    backpack.pbrMetallic.albedo = backpackAlbedoId;
    backpack.pbrMetallic.roughness = backpackRoughId;
    backpack.pbrMetallic.metallic = backpackMetallicId;
    backpack.pbrMetallic.normals = backpackNormalId;

    Material checkerboard = {};
    checkerboard.workflow = Material::Phong;
    checkerboard.phong.diffuse = checkerboardID;

    auto checkerboardEntity = GetEntity(world, checkerboardEntityID);
    checkerboardEntity->mesh = GetID(&assetManager->nameTable, "plate");
    assert(checkerboardEntity->mesh);
    checkerboardEntity->material = checkerboard;

    auto backpackEntity = GetEntity(world, backpackEntityID);
    backpackEntity->p = V3(1.0f);
    backpackEntity->scale = V3(0.01f);
    backpackEntity->mesh = GetID(&assetManager->nameTable, "backpack_low");
    assert(backpackEntity->mesh);
    backpackEntity->material = backpack;

    auto sphereEntity = GetEntity(world, sphereEntityID);
    sphereEntity->mesh = GetID(&assetManager->nameTable, "sphere");
    assert(sphereEntity->mesh);
    sphereEntity->material = oldMetal;
}

void FluxReload(Context* context) {
}

void FluxUpdate(Context* context) {
    auto ui = &context->ui;
    auto world = context->world;
    auto renderer = context->renderer;
    auto assetManager = &context->assetManager;

    DEBUG_OVERLAY_TRACE(assetManager->assetQueueUsage);
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
        auto newWorld = LoadWorldFrom(ui, assetManager);
        if (newWorld) {
            // TODO: Loading and unloading levels
            // TODO: Get rid if this random deallocation confusion
            Drop(&context->world->entityTable);
            PlatformFree(context->world);
            ui->selectedEntity = 0;
            context->world = newWorld;
            world = newWorld;
        }
    }

    if (ui->wantsSaveAs) {
        SaveWorldAs(ui, world, assetManager);
    }

    if (ui->wantsAddEntity) {
        ui->wantsAddEntity = false;
        auto entity = AddEntity(world);
        entity->mesh = GetID(&assetManager->nameTable, "../res/meshes/sphere.aab");
        // TODO: Assign material
        entity->material = {};
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
            if (SaveToDisk(assetManager, world, buffer)) {
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
        auto entity = Get(&world->entityTable, &ui->selectedEntity);
        assert(entity);
        auto mesh = GetMesh(assetManager, entity->mesh);
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

    for (Entity& entity : context->world->entityTable) {
        assert(entity.id);
        if (entity.id) {
            auto mesh = GetMesh(assetManager, entity.mesh);
            if (mesh) {
                RenderCommandDrawMesh command = {};
                command.transform = entity.transform;
                command.meshID = entity.mesh;
                command.material = entity.material;
                Push(group, &command);
                if (context->ui.showBoundingVolumes) {
                    auto aabb = mesh->aabb;
                    aabb.min = (entity.transform * V4(aabb.min, 1.0f)).xyz;
                    aabb.max = (entity.transform * V4(aabb.max, 1.0f)).xyz;
                    DrawAlignedBoxOutline(&context->renderGroup, aabb.min, aabb.max, V3(1.0f, 0.0f, 0.0f), 2.0f);
                }
            }
        }
    }

    Begin(renderer, group);
    ShadowPass(renderer, group, assetManager);
    MainPass(renderer, group, assetManager);
    End(renderer);

    // Alpha
    //ImGui::PopStyleVar();
}

void FluxRender(Context* context) {}
