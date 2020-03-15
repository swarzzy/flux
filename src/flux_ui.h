#pragma once

struct Ui {
    b32 entityListerOpen;
    b32 entityInspectorOpen;
    b32 assetManagerOpen;
    b32 uniformEntityScale;
    b32 wantsAddEntity;
    b32 wantsSave;
    b32 wantsLoadLoadFrom;
    b32 wantsSaveAs;
    u32 showBoundingVolumes;
    u32 showDebugOverlay = 1;
    u32 selectedEntity;
    u32 selectedResource;
    MeshFileFormat meshLoadFormat;
    char assetLoadFileBuffer[MaxAssetPathSize];
    char worldNameBuffer[128];
    wchar_t worldNameBufferW[128];
};

static_assert(array_count(typedecl(Ui, worldNameBuffer)) == array_count(typedecl(Ui, worldNameBufferW)));

World* LoadWorldFrom(Ui* ui, AssetManager* assetManager);
void SaveWorldAs(Ui* ui, World* world, AssetManager* assetManager);

bool IsMouseCapturedByUI();
bool IsKeyboardCapturedByUI();
void UpdateUi(Context* context);
