#include "flux.h"
#include "flux_platform.h"
#include "flux_debug_overlay.h"


void FluxInit(Context* context) {
    context->wheelMesh = LoadMesh("../res/meshes/backpack_low.fbx");
    context->sphereMesh = LoadMeshAAB(L"../res/meshes/sphere.aab");
    context->plateMesh = LoadMeshAAB(L"../res/meshes/plate.aab");
    context->checkerboardMaterial = LoadMaterialLegacy("../res/checkerboard.jpg");
    context->oldMetalMaterial = LoadMaterialPBRMetallic("../res/materials/oldmetal/greasy-metal-pan1-albedo.png", "../res/materials/oldmetal/greasy-metal-pan1-roughness.png", "../res/materials/oldmetal/greasy-metal-pan1-metal.png", "../res/materials/oldmetal/greasy-metal-pan1-normal.png");
    context->backpackMaterial = LoadMaterialPBRMetallic("../res/materials/backpack/albedo.png", "../res/materials/backpack/rough.png", "../res/materials/backpack/metallic.png", "../res/materials/backpack/normal.png");
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

    auto checkerboardEntity = AddEntity(&context->world);
    auto backpackEntity = AddEntity(&context->world);
    auto sphereEntity = AddEntity(&context->world);

    checkerboardEntity->mesh = &context->plateMesh;
    checkerboardEntity->material = &context->checkerboardMaterial;

    backpackEntity->p = V3(1.0f);
    backpackEntity->scale = V3(0.01f);
    backpackEntity->mesh = context->wheelMesh;
    backpackEntity->material = &context->backpackMaterial;

    sphereEntity->mesh = &context->sphereMesh;
    sphereEntity->material = &context->oldMetalMaterial;
}

void FluxReload(Context* context) {

}

void DrawMenu(Ui* ui) {
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Entity lister")) {
            ui->entityListerOpen = true;
        }
        if (ImGui::MenuItem("Entity inspector")) {
            ui->entityInspectorOpen = true;
        }
        if (ImGui::MenuItem("Show bounding volumes", nullptr, ui->showBoundingVolumes)) {
            ui->showBoundingVolumes = !ui->showBoundingVolumes;
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void DrawEntityLister(Context* context) {
    auto ui = &context->ui;
    ImGuiIO* io = &ImGui::GetIO();
    ImGui::SetNextWindowSize({200, 400});
    auto windowFlags = ImGuiWindowFlags_NoResize; //ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("Entity lister", (bool*)&ui->entityListerOpen, windowFlags)) {
        ImGui::BeginChild("Entity lister list");
        for (u32 i = 0 ; i < array_count(context->world.entities); i++)
        {
            auto e = context->world.entities + i;
            if (e->id) {
                bool wasSelected = (e->id == ui->selectedEntity);
                char buffer[128];
                sprintf_s(buffer, 128, "id: %lu", (uint)e->id);
                bool selected = ImGui::Selectable(buffer, wasSelected);
                if (selected) {
                    ui->selectedEntity = e->id;
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void DrawEntityInspector(Ui* ui, World* world) {
    auto io = ImGui::GetIO();
    ImGui::SetNextWindowSize({300, 600});
    auto windowFlags = ImGuiWindowFlags_NoResize; //ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("Entity inspector", (bool*)&ui->entityInspectorOpen, windowFlags)) {
        if (!ui->selectedEntity)  {
            ImGui::Text("No entity selected");
        } else {
            auto entity = GetEntity(world, ui->selectedEntity);

            char buffer[16];
            sprintf_s(buffer, 16, "%lu", (uint)ui->selectedEntity);
            ImGui::Text("id: %s", buffer);
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                ui->selectedEntity = 0;
                DeleteEntity(world, entity->id);
            } else {
                ImGui::Separator();
                ImGui::Text("Position");
                ImGui::PushID("Entity position drag");
                ImGui::DragFloat3("", entity->p.data);
                ImGui::PopID();

                ImGui::Separator();
                ImGui::Text("Scale");
                bool uniform = ui->uniformEntityScale;
                ImGui::Checkbox("Uniform scale", &uniform);
                ui->uniformEntityScale = uniform;
                ImGui::PushID("Entity scale drag");
                if (ui->uniformEntityScale) {
                    ImGui::DragFloat("", &entity->scale.x);
                    entity->scale.y = entity->scale.x;
                    entity->scale.z = entity->scale.x;
                } else {
                    ImGui::DragFloat3("", entity->scale.data);
                }
                ImGui::PopID();

                ImGui::Separator();
                ImGui::Text("Mesh: %s", entity->mesh->name);
            }
        }
    }
    ImGui::End();
}

void FluxUpdate(Context* context) {
    Update(&context->camera, GlobalAbsDeltaTime);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawDebugPerformanceCounters();
    DrawMenu(&context->ui);

    if (context->ui.entityListerOpen) {
        DrawEntityLister(context);
    }
    if (context->ui.entityInspectorOpen) {
        DrawEntityInspector(&context->ui, &context->world);
    }

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

    for (uint i = 0; i < array_count(context->world.entities); i++) {
        auto entity = context->world.entities + i;
        if (entity->id) {
            RenderCommandDrawMesh command = {};
            auto transform = Translate(entity->p) * Scale(entity->scale);
            command.transform = transform;
            command.mesh = entity->mesh;
            command.material = *entity->material;
            Push(group, &command);
            if (context->ui.showBoundingVolumes) {
                auto aabb = entity->mesh->aabb;
                aabb.min = (transform * V4(aabb.min, 1.0f)).xyz;
                aabb.max = (transform * V4(aabb.max, 1.0f)).xyz;
                DrawAlignedBoxOutline(&context->renderGroup, aabb.min, aabb.max, V3(1.0f, 0.0f, 0.0f), 2.0f);
            }
        }
    }

    Begin(renderer, group);
    ShadowPass(renderer, group);
    MainPass(renderer, group);
    End(renderer);
}

void FluxRender(Context* context) {}
