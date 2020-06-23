#include "flux.h"
#include "../flux-platform/src/Common.h"
#include "flux_debug_overlay.h"
#include "flux_resource_manager.h"

void FluxInit(Context* context) {
    AssetManager::Init(&context->assetManager, context->renderer);

    context->tempArena = AllocateArena(Megabytes(32), true);

    StringBuilderW builder {};
    {
        StringBuilderInit(&builder, MakeAllocator(PlatformAlloc, PlatformFree, nullptr));

        wchar_t from[] = L"E:\\Dev/flux\\build\\flux.exe";
        wchar_t to [] = L"E:\\dev\\flux\\res\\sponza\\floor.mesh";

        NormalizePath(from);
        NormalizePath(to);

        auto result = GetRelativePath(&builder, from, to);
    }
    {
        StringBuilderInit(&builder, MakeAllocator(PlatformAlloc, PlatformFree, nullptr));

        wchar_t from[] = L"E:\\Dev/flux\\";
        wchar_t to [] = L"E:\\dev\\flux\\res\\sponza\\";

        NormalizePath(from);
        NormalizePath(to);

        auto result = GetRelativePath(&builder, from, to);
    }
    {
        StringBuilderInit(&builder, MakeAllocator(PlatformAlloc, PlatformFree, nullptr));

        wchar_t from[] = L"C:\\Dev/flux\\build\\flux.exe";
        wchar_t to [] = L"E:\\dev\\flux\\res\\sponza\\floor.mesh";

        NormalizePath(from);
        NormalizePath(to);

        auto result = GetRelativePath(&builder, from, to);
    }

    StringBuilder cbuilder {};
    StringBuilderInit(&cbuilder, MakeAllocator(PlatformAlloc, PlatformFree, nullptr));

    for (u32 i = 0; i < 100; i++) {
        char buffer[256];
        sprintf(buffer, "This is string %lu", i);
        StringBuilderAppend(&cbuilder, buffer);
    }


    context->hdrMap = LoadCubemapHDR("../res/desert_sky/nz.hdr", "../res/desert_sky/ny.hdr", "../res/desert_sky/pz.hdr", "../res/desert_sky/nx.hdr", "../res/desert_sky/px.hdr", "../res/desert_sky/py.hdr");
    UploadToGPU(&context->hdrMap);
    context->irradanceMap = MakeEmptyCubemap(64, 64, TextureFormat::RGB16F, TextureFilter::Bilinear, TextureWrapMode::ClampToEdge, false);
    UploadToGPU(&context->irradanceMap);
    context->enviromentMap = MakeEmptyCubemap(256, 256, TextureFormat::RGB16F, TextureFilter::Trilinear, TextureWrapMode::ClampToEdge, true);
    UploadToGPU(&context->enviromentMap);

    context->renderGroup.drawSkybox = true;
    context->renderGroup.skyboxHandle = context->enviromentMap.gpuHandle;
    context->renderGroup.irradanceMapHandle = context->irradanceMap.gpuHandle;
    context->renderGroup.envMapHandle = context->enviromentMap.gpuHandle;

    GenIrradanceMap(context->renderer, &context->irradanceMap, context->hdrMap.gpuHandle);
    GenEnvPrefiliteredMap(context->renderer, &context->enviromentMap, context->hdrMap.gpuHandle, 6);

    auto defaultWorld = LoadWorldFromDisc(&context->assetManager, DefaultWorldW);
    if (defaultWorld) {
        wprintf(L"[Flux] Loaded default world: %ls", DefaultWorldW);
        context->world = defaultWorld;
    } else {
        context->world = (World*)PlatformAlloc(sizeof(World), 0, nullptr);
        *context->world = {};
        strcpy_s(context->world->name, array_count(context->world->name), DefaultWorld);

        auto world = context->world;
        auto assetManager = &context->assetManager;

        AddMesh(assetManager, "../res/meshes/plate.aab", MeshFileFormat::AAB);
        AddMesh(assetManager, "../res/meshes/sphere.aab", MeshFileFormat::AAB);
        AddMesh(assetManager, "../res/meshes/backpack_low.mesh", MeshFileFormat::Flux);

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
        oldMetal.pbrMetallic.useAlbedoMap = true;
        oldMetal.pbrMetallic.useRoughnessMap = true;
        oldMetal.pbrMetallic.useMetallicMap = true;
        oldMetal.pbrMetallic.useNormalMap = true;
        oldMetal.pbrMetallic.normalFormat = NormalFormat::DirectX;
        oldMetal.pbrMetallic.albedoMap = oldMetalAlbedoId;
        oldMetal.pbrMetallic.roughnessMap = oldMetalRoughId;
        oldMetal.pbrMetallic.metallicMap = oldMetalMetallicId;
        oldMetal.pbrMetallic.normalMap = oldMetalNormalId;

        Material backpack = {};
        backpack.workflow = Material::PBRMetallic;
        backpack.pbrMetallic.useAlbedoMap = true;
        backpack.pbrMetallic.useRoughnessMap = true;
        backpack.pbrMetallic.useMetallicMap = true;
        backpack.pbrMetallic.useNormalMap = true;
        backpack.pbrMetallic.normalFormat = NormalFormat::DirectX;
        backpack.pbrMetallic.albedoMap = backpackAlbedoId;
        backpack.pbrMetallic.roughnessMap = backpackRoughId;
        backpack.pbrMetallic.metallicMap = backpackMetallicId;
        backpack.pbrMetallic.normalMap = backpackNormalId;

        Material checkerboard = {};
        checkerboard.workflow = Material::Phong;
        checkerboard.phong.useDiffuseMap = true;
        checkerboard.phong.diffuseMap = checkerboardID;

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
}

void FluxReload(Context* context) {
}

void FluxUpdate(Context* context) {
    if (context->showConsole) {
        DrawConsole(&context->console);
    }

    auto ui = &context->ui;
    auto world = context->world;
    auto renderer = context->renderer;
    auto assetManager = &context->assetManager;

    if (KeyPressed(Key::Tilde)) {
        context->showConsole = !context->showConsole;
    }

    i32 rendererSampleCount = GetRenderSampleCount(renderer);
    DEBUG_OVERLAY_SLIDER(rendererSampleCount, 0, GetRenderMaxSampleCount(renderer));
    if (rendererSampleCount != GetRenderSampleCount(renderer)) {
        ChangeRenderResolution(renderer, GetRenderResolution(renderer), rendererSampleCount);
    }

    DEBUG_OVERLAY_TRACE(assetManager->assetQueueUsage);
    CompletePendingLoads(assetManager);

    auto renderRes = GetRenderResolution(renderer);
    if (renderRes.x != GlobalPlatform.windowWidth ||
        renderRes.y != GlobalPlatform.windowHeight) {
        ChangeRenderResolution(renderer, UV2(GlobalPlatform.windowWidth, GlobalPlatform.windowHeight), GetRenderSampleCount(renderer));
    }


    Update(&context->camera, 1.0f / 60.0f);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //DrawDebugPerformanceCounters();

    UpdateUi(context);

    if (ui->wantsLoadLoadFrom) {
        auto newWorld = LoadWorldFrom(ui, assetManager);
        if (newWorld) {
            // TODO: Loading and unloading levels
            // TODO: Get rid if this random deallocation confusion
            Drop(&context->world->entityTable);
            PlatformFree(context->world, nullptr);
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

    auto entity = GetEntity(world, 11);
    if (entity) {
        entity->rotationAngles.y += GlobalGameDeltaTime * 50.0f;
        if (entity->rotationAngles.y >= 360.0f) {
            entity->rotationAngles.y = 0.0f;
        }
    }

    Update(world);

    auto group = &context->renderGroup;

    group->camera = &context->camera;
    auto camera = &context->camera;

    DirectionalLight light = {};
    light.dir = Normalize(V3(0.3f, -1.0f, -0.95f));
    light.from = V3(4.0f, 200.0f, 0.0f);
    light.ambient = V3(0.3f);
    light.diffuse = V3(5.8f);
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
