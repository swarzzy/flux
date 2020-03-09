#include "flux_ui.h"

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
        if (ImGui::MenuItem("Show debug overlay", nullptr, ui->showDebugOverlay)) {
            ui->showDebugOverlay = !ui->showDebugOverlay;
        }
        if (ImGui::MenuItem("Save world")) {
            ui->wantsSave = true;
        }
        if (ImGui::MenuItem("Load from")) {
            ui->wantsLoadLoadFrom = true;
        }
        if (ImGui::MenuItem("Save as")) {
            ui->wantsSaveAs = true;
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
        for (u32 i = 0 ; i < array_count(context->world->entities); i++)
        {
            auto e = context->world->entities + i;
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
        if (ImGui::Button("Add entity")) {
            ui->wantsAddEntity = true;
        }
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
                //ImGui::Text("Mesh: %s", context->meshes[(u32)entity->mesh]->name);
            }
        }
    }
    ImGui::End();
}

// TODO: Factor loading logic out if this
World* LoadWorldFrom(Ui* ui) {
    World* world = nullptr;
    if (!ImGui::IsPopupOpen("Load world from")) {
        ImGui::OpenPopup("Load world from");
    }
    if (ImGui::BeginPopupModal("Load world from", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushID("Load world from filename input");
        if (ImGui::InputText("", ui->worldNameBuffer, array_count(ui->worldNameBuffer))) {
            mbstowcs(ui->worldNameBufferW, ui->worldNameBuffer, array_count(ui->worldNameBuffer));
        }
        ImGui::PopID();

        if (ImGui::Button("Load")) {
            world = LoadWorldFromDisc(ui->worldNameBufferW);
            if (world) {
                strcpy_s(world->name, array_count(world->name), ui->worldNameBuffer);
                ui->wantsLoadLoadFrom = false;
                ui->worldNameBuffer[0] = 0;
                ui->worldNameBufferW[0] = 0;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Close")) {
            ui->wantsLoadLoadFrom = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return world;
}

// TODO: Factor saving logic out of this
void SaveWorldAs(Ui* ui, World* world) {
    if (!ImGui::IsPopupOpen("Save world as")) {
        ImGui::OpenPopup("Save world as");
    }
    if (ImGui::BeginPopupModal("Save world as", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushID("Save world as filename input");
        if (ImGui::InputText("", ui->worldNameBuffer, array_count(ui->worldNameBuffer))) {
            mbstowcs(ui->worldNameBufferW, ui->worldNameBuffer, array_count(ui->worldNameBuffer));
        }
        ImGui::PopID();

        if (ImGui::Button("Save")) {
            if (!PlatformDebugGetFileSize(ui->worldNameBufferW)) {
                if (SaveToDisk(world, ui->worldNameBufferW)) {
                    strcpy_s(world->name, array_count(world->name), ui->worldNameBuffer);
                    ui->wantsSaveAs = false;
                    ui->worldNameBuffer[0] = 0;
                    ui->worldNameBufferW[0] = 0;
                    ImGui::CloseCurrentPopup();
                }
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Close")) {
        ui->wantsSaveAs = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

bool IsMouseCapturedByUI() {
    ImGuiIO* io = &ImGui::GetIO();
    bool result = io->WantCaptureMouse;
    return result;
}

bool IsKeyboardCapturedByUI() {
    ImGuiIO* io = &ImGui::GetIO();
    bool result = io->WantCaptureKeyboard;
    return result;
}

void UpdateUi(Context* context) {
    auto ui = &context->ui;
    auto world = context->world;

    DrawMenu(ui);

    GlobalDrawDebugOverlay = ui->showDebugOverlay;

    if (ui->entityListerOpen) {
        DrawEntityLister(context);
    }
    if (ui->entityInspectorOpen) {
        DrawEntityInspector(ui, world);
    }

    auto rd = context->camera.mouseRay;
    auto ro = context->camera.position;
    if (!IsMouseCapturedByUI() && !IsKeyboardCapturedByUI()) {
        if (MouseButtonPressed(MouseButton::Left)) {
            DEBUG_OVERLAY_TRACE(context->camera.position);
            auto raycast = Raycast(&context->assetManager, world, ro, rd);
            if (raycast) {
                ui->selectedEntity = raycast.Unwrap().entityId;
            }
        }
    }
}
