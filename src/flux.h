#pragma once
#include "flux_camera.h"
#include "flux_renderer.h"
#include "flux_render_group.h"

struct Entity {
    u32 id;
    v3 p;
    v3 scale = V3(1.0f);
    Mesh* mesh;
    Material* material;
};

struct World {
    u32 nextEntitySerialNumber = 1;
    Entity entities[32];
};

Entity* FindEntry(World* world, u32 id) {
    Entity* result = nullptr;
    u32 hashMask = array_count(world->entities) - 1;
    auto hash = id & hashMask;
    for (u32 offset = 0; offset < array_count(world->entities); offset++) {
        u32 index = (hash + offset) & hashMask;
        auto entry = world->entities + index;
        if (entry->id == id) {
            result = entry;
            break;
        }
    }
    return result;
}

Entity* AddEntity(World* world) {
    Entity* result = nullptr;
    auto entry = FindEntry(world, 0);
    if (entry) {
        auto id = world->nextEntitySerialNumber++;
        *entry = {};
        entry->id = id;
        result = entry;
    }
    return result;
}

Entity* GetEntity(World* world, u32 id) {
    Entity* result = nullptr;
    if (id) {
        auto entry = FindEntry(world, id);
        if (entry) {
            result = entry;
        }
    }
    return result;
}

bool DeleteEntity(World* world, u32 id) {
    bool result = false;
    if (id) {
        auto entry = FindEntry(world, id);
        if (entry) {
            entry->id = 0;
            result = true;
        }
    }
    return result;
}

struct Ui {
    b32 entityListerOpen;
    b32 entityInspectorOpen;
    b32 uniformEntityScale;
    u32 showBoundingVolumes;
    u32 selectedEntity;
};

struct Context {
    World world;
    Ui ui;
    GLuint prog;
    GLuint vbo;
    Camera camera;
    Renderer* renderer;
    RenderGroup renderGroup;
    Mesh sphereMesh;
    Mesh plateMesh;
    Mesh* wheelMesh;
    Material oldMetalMaterial;
    Material checkerboardMaterial;
    Material backpackMaterial;
    CubeTexture skybox;
    CubeTexture hdrMap;
    CubeTexture irradanceMap;
    CubeTexture enviromentMap;
};

void FluxInit(Context* context);
void FluxReload(Context* context);
void FluxUpdate(Context* context);
void FluxRender(Context* context);
