#pragma once

#include "flux_platform.h"

enum struct EntityMaterial {
    Checkerboard = 0,
    OldMetal,
    Backpack,
    _Count
};

enum struct EntityMesh {
    Sphere = 0,
    Plate,
    Backpack,
    Gizmos,
    _Count
};

// TODO: This is tempoary solution
struct MaterialSlot {
    enum { NotLoaded = 0, Loading, JustLoaded, Ready } state;
    Material material;
};

struct MeshSlot {
    enum { NotLoaded = 0, Loading, JustLoaded, Ready } state;
    Mesh* mesh;
};

struct AssetManager {
    MeshSlot meshes[EntityMesh::_Count];
    MaterialSlot materials[EntityMaterial::_Count];
};

Mesh* Get(AssetManager* manager, EntityMesh id);
Material* Get(AssetManager* manager, EntityMaterial id);

Mesh* LoadMeshFlux(const wchar_t* filename);
