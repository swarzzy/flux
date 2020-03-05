#pragma once

struct Ui {
    b32 entityListerOpen;
    b32 entityInspectorOpen;
    b32 uniformEntityScale;
    b32 wantsAddEntity;
    b32 wantsSave;
    b32 wantsLoadLoadFrom;
    b32 wantsSaveAs;
    u32 showBoundingVolumes;
    u32 showDebugOverlay;
    u32 selectedEntity;
    char worldNameBuffer[128];
    wchar_t worldNameBufferW[128];
};

static_assert(array_count(typedecl(Ui, worldNameBuffer)) == array_count(typedecl(Ui, worldNameBufferW)));

World* LoadWorldFrom(Ui* ui);
void SaveWorldAs(Ui* ui, World* world);

bool IsMouseCapturedByUI();
bool IsKeyboardCapturedByUI();
void UpdateUi(Context* context);
