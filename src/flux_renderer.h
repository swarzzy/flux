#pragma once

#include "flux_resource_manager.h"

struct Renderer;


Renderer* InitializeRenderer(uv2 renderRes);
Mesh* LoadMeshAAB(const wchar_t* filepath);

void GenIrradanceMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle);
void GenEnvPrefiliteredMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle, u32 mipLevels);

// TODO: Temporary hack while struct is defined in cpp file
uv2 GetRenderResolution(Renderer* renderer);
void ChangeRenderResolution(Renderer* renderer, uv2 newRes);

struct RenderGroup;

void Begin(Renderer* renderer, RenderGroup* group);
void ShadowPass(Renderer* renderer, RenderGroup* group);
void MainPass(Renderer* renderer, RenderGroup* group);
void End(Renderer* renderer);

void UploadToGPU(CubeTexture* texture);
void UploadToGPU(Mesh* mesh);
void UploadToGPU(Texture* texture);

void RecompileShaders(Renderer* renderer);
