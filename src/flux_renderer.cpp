#include "flux_renderer.h"

#include "flux_std140.h"
#include "flux_shaders.h"

struct Renderer {
    union {
        Shaders shaders;
        GLuint shaderHandles[ShaderCount];
    };

    GLuint lineBufferHandle;
    GLuint chunkIndexBuffer;
    v4 clearColor;

    uv2 renderRes;
    f32 gamma = 2.4f;
    f32 exposure = 1.0f;

    GLuint offscreenBufferHandle;
    GLuint offscreenColorTarget;
    GLuint offscreenDepthTarget;

    static constexpr u32 RandomValuesTextureSize = 1024;
    // NOTE: Number of cascades is always 3
    static constexpr u32 NumShadowCascades = 3;
    GLuint shadowMapFramebuffers[NumShadowCascades];
    GLuint shadowMapDepthTarget;
    GLuint shadowMapDebugColorTarget;
    u32 shadowMapRes = 1024;
    GLuint randomValuesTexture;
    b32 stableShadows = true;
    b32 showShadowCascadesBoundaries;
    f32 shadowFilterScale = 1.0f;

    // TODO: Camera should cares about that
    m4x4 shadowCascadeViewProjMatrices[NumShadowCascades];
    f32 shadowCascadeBounds[NumShadowCascades];

    f32 shadowConstantBias = 0.004f;
    f32 shadowSlopeBiasScale = 1.2f;
    f32 shadowNormalBiasScale;

    GLuint srgbBufferHandle;
    GLuint srgbColorTarget;

    GLfloat maxAnisotropy;

    GLuint captureFramebuffer;

    GLuint BRDFLutHandle;

    b32 debugF;
    b32 debugD;
    b32 debugG;
    b32 debugNormals;

    u32 uniformBufferAligment;

    UniformBuffer<ShaderFrameData, ShaderFrameData::Binding> frameUniformBuffer;
    UniformBuffer<ShaderMeshData, ShaderMeshData::Binding> meshUniformBuffer;
};

// TODO: Refactor these
void RendererLoadTexture(Texture* texture) {
    if (!texture->gpuHandle) {
        GLuint handle;
        glGenTextures(1, &handle);
        if (handle) {
            glBindTexture(GL_TEXTURE_2D, handle);
            //glEnable(GL_TEXTURE_2D);
            GLenum format;
            GLenum type;
            GLenum minFilter = GL_NEAREST;
            GLenum magFilter = GL_NEAREST;
            bool anisotropic = false;
            GLenum wrapMode = texture->wrapMode;

            switch (texture->format) {
            case GL_SRGB8_ALPHA8: { format = GL_RGBA; type = GL_UNSIGNED_BYTE; } break;
            case GL_SRGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
            case GL_RGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
            case GL_RGB16F: { format = GL_RGB; type = GL_FLOAT; } break;
            case GL_RG16F: { format = GL_RG, type = GL_FLOAT; } break;
            case GL_R8: {format = GL_RED, type = GL_UNSIGNED_BYTE; } break;
                invalid_default();
            }

            switch (texture->filter) {
            case TextureFilter::Bilinear: { minFilter = GL_LINEAR; magFilter = GL_LINEAR; } break;
            case TextureFilter::Trilinear: { minFilter = GL_LINEAR_MIPMAP_LINEAR; magFilter = GL_LINEAR; } break;
            case TextureFilter::Anisotropic: {minFilter = GL_LINEAR_MIPMAP_LINEAR; magFilter = GL_LINEAR; anisotropic = true; } break;
                invalid_default();
            }

            glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width,
                         texture->height, 0, format, type, texture->data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            if (anisotropic) {
                // TODO: Anisotropy value
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_ARB, 8.0f);
            }

            glGenerateMipmap(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, 0);

            texture->gpuHandle = handle;
        }
    }
}

void RendererLoadCubeTexture(CubeTexture* texture) {
    if (!texture->gpuHandle) {
        GLuint handle;
        glGenTextures(1, &handle);
        if (handle) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            auto minFilter = GL_NEAREST;
            auto magFilter = GL_NEAREST;

            switch (texture->filter) {
            case TextureFilter::None: {} break;
            case TextureFilter::Bilinear: { minFilter = GL_LINEAR; magFilter = GL_LINEAR; } break;
            case TextureFilter::Trilinear: { if (texture->useMips) minFilter = GL_LINEAR_MIPMAP_LINEAR; else minFilter = GL_LINEAR; magFilter = GL_LINEAR; } break;
                invalid_default();
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);

            for (u32 i = 0; i < 6; i++) {
                GLenum internalFormat = texture->images[i].format;
                GLenum format;
                GLenum type;

                u32 width = texture->images[i].width;
                u32 height = texture->images[i].height;
                void* data = texture->images[i].data;

                switch (internalFormat) {
                case GL_SRGB8_ALPHA8: { format = GL_RGBA; type = GL_UNSIGNED_BYTE; } break;
                case GL_SRGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
                case GL_RGB16F: {format = GL_RGB; type = GL_FLOAT; } break;
                    invalid_default();
                }

                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                             internalFormat, width, height, 0,
                             format, type, data);
            }

            if (texture->useMips) {
                glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            }

            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

            texture->gpuHandle = handle;
        }
    }
}

void RendererLoadMesh(Mesh* mesh) {
    while (mesh) {
        if (!mesh->gpuVertexBufferHandle && !mesh->gpuIndexBufferHandle) {
            GLuint vboHandle;
            GLuint iboHandle;
            glGenBuffers(1, &vboHandle);
            glGenBuffers(1, &iboHandle);
            if (vboHandle && iboHandle) {
                // NOTE: Using SOA layout of buffer
                uptr verticesSize = mesh->vertexCount * sizeof(v3);
                // TODO: this is redundant. Use only vertexCount
                uptr normalsSize = mesh->vertexCount * sizeof(v3);
                uptr uvsSize = mesh->vertexCount * sizeof(v2);
                uptr tangentsSize = mesh->vertexCount * sizeof(v3);
                uptr indexBufferSize = mesh->indexCount * sizeof(u32);
                uptr vertexBufferSize = verticesSize + normalsSize + uvsSize + tangentsSize;

                glBindBuffer(GL_ARRAY_BUFFER, vboHandle);

                glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, 0, GL_STATIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, verticesSize, (void*)mesh->vertices);
                glBufferSubData(GL_ARRAY_BUFFER, verticesSize, normalsSize, (void*)mesh->normals);
                glBufferSubData(GL_ARRAY_BUFFER, verticesSize + normalsSize, uvsSize, (void*)mesh->uvs);
                glBufferSubData(GL_ARRAY_BUFFER, verticesSize + normalsSize + uvsSize, tangentsSize, (void*)mesh->tangents);

                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboHandle);

                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, (void*)mesh->indices, GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                mesh->gpuVertexBufferHandle = vboHandle;
                mesh->gpuIndexBufferHandle = iboHandle;
            }
        }
        mesh = mesh->next;
    }
}

CubeTexture LoadCubemap(const char* backPath, const char* downPath, const char* frontPath,
                        const char* leftPath, const char* rightPath, const char* upPath,
                        DynamicRange range, GLenum glFormat) {
    CubeTexture texture = {};

    // TODO: Use memory arena
    auto back = ResourceLoaderLoadImage(backPath, range, false, 0, PlatformAlloc);
    defer { PlatformFree(back->base); };
    auto down = ResourceLoaderLoadImage(downPath, range, false, 0, PlatformAlloc);
    defer { PlatformFree(down->base); };
    auto front = ResourceLoaderLoadImage(frontPath, range, false, 0, PlatformAlloc);
    defer { PlatformFree(front->base); };
    auto left = ResourceLoaderLoadImage(leftPath, range, false, 0, PlatformAlloc);
    defer { PlatformFree(left->base); };
    auto right = ResourceLoaderLoadImage(rightPath, range, false, 0, PlatformAlloc);
    defer { PlatformFree(right->base); };
    auto up = ResourceLoaderLoadImage(upPath, range, false, 0, PlatformAlloc);
    defer { PlatformFree(up->base); };

    texture.back.format = glFormat;
    texture.back.width = back->width;
    texture.back.height = back->height;
    texture.back.data = back->bits;

    texture.down.format = glFormat;
    texture.down.width = down->width;
    texture.down.height = down->height;
    texture.down.data = down->bits;

    texture.front.format = glFormat;
    texture.front.width = front->width;
    texture.front.height = front->height;
    texture.front.data = front->bits;

    texture.left.format = glFormat;
    texture.left.width = left->width;
    texture.left.height = left->height;
    texture.left.data = left->bits;

    texture.right.format = glFormat;
    texture.right.width = right->width;
    texture.right.height = right->height;
    texture.right.data = right->bits;

    texture.up.format = glFormat;
    texture.up.width = up->width;
    texture.up.height = up->height;
    texture.up.data = up->bits;

    RendererLoadCubeTexture(&texture);
    assert(texture.gpuHandle);

    return texture;
}

CubeTexture LoadCubemap(const char* backPath, const char* downPath, const char* frontPath,
                        const char* leftPath, const char* rightPath, const char* upPath) {
    return LoadCubemap(backPath, downPath, frontPath, leftPath, rightPath, upPath, DynamicRange::LDR, GL_SRGB8);
}

CubeTexture LoadCubemapHDR(const char* backPath, const char* downPath, const char* frontPath,
                           const char* leftPath, const char* rightPath, const char* upPath) {
    return LoadCubemap(backPath, downPath, frontPath, leftPath, rightPath, upPath, DynamicRange::HDR, GL_RGB16F);
}


CubeTexture MakeEmptyCubemap(int w, int h, GLenum format, TextureFilter filter, bool useMips) {
    CubeTexture texture = {};
    texture.useMips = useMips;
    texture.filter = filter;
    for (u32 i = 0; i < 6; i++) {
        texture.images[i].format = format;
        texture.images[i].width = w;
        texture.images[i].height = h;
        texture.images[i].data = 0;
    }
    RendererLoadCubeTexture(&texture);
    assert(texture.gpuHandle);
    return texture;
}

int STBDesiredBPPFromGLFormat(GLenum format) {
    int desiredBpp = 0;
    switch (format) {
    case GL_SRGB8_ALPHA8:
    case GL_RGBA: { desiredBpp = 4; } break;
    case GL_SRGB8:
    case GL_RGB8:
    case GL_RGB16F: { desiredBpp = 3; } break;
    case GL_RG16F: { desiredBpp = 2; } break;
    case GL_R8: { desiredBpp = 1; } break;
        invalid_default();
    }
    return desiredBpp;
}

GLenum GLTexFormatFromNumChannels(u32 num) {
    GLenum format = 0;
    switch (num) {
    case 1: { format = GL_R8; } break;
    case 2: { format = GL_RG8; } break; // TODO: Implement in renderer
    case 3: { format = GL_RGB8; } break;
    case 4: { format = GL_SRGB8_ALPHA8; } break;
        invalid_default();
    }
    return format;
}

Texture LoadTexture(const char* filename,
                    GLenum format = 0,
                    GLenum wrapMode = GL_REPEAT,
                    TextureFilter filter = TextureFilter::Bilinear) {
    Texture t = {};

    i32 desiredBpp = 0;

    if (format) {
        desiredBpp = STBDesiredBPPFromGLFormat(format);
    }

    auto image = ResourceLoaderLoadImage(filename, DynamicRange::LDR, true, desiredBpp, PlatformAlloc);
    assert(image);

    if (!format) {
        format = GLTexFormatFromNumChannels(image->channels);
    }

    t.format = format;
    t.width = image->width;
    t.height = image->height;
    t.wrapMode = wrapMode;
    t.filter = filter;

    t.data = image->bits;
    RendererLoadTexture(&t);
    assert(t.gpuHandle);

    return t;
}

Texture LoadTextureAsync(const char* filename, GLenum format = 0, GLenum wrapMode = GL_REPEAT, TextureFilter filter = TextureFilter::Bilinear) {
    Texture t = {};
    i32 desiredBpp = 0;
    if (format) {
        desiredBpp = STBDesiredBPPFromGLFormat(format);
    }

    auto image = ResourceLoaderLoadImage(filename, DynamicRange::LDR, true, desiredBpp, PlatformAlloc);
    assert(image);

    if (!format) {
        format = GLTexFormatFromNumChannels(image->channels);
    }

    t.format = format;
    t.width = image->width;
    t.height = image->height;
    t.wrapMode = wrapMode;
    t.filter = filter;
    t.data = image->bits;
    return t;
}

Material LoadMaterialPBRMetallicAsync(const char* albedoPath, const char* roughnessPath, const char* metalnessPath, const char* normalsPath) {
    Texture albedo = LoadTextureAsync(albedoPath, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture roughness = LoadTextureAsync(roughnessPath, GL_R8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture metalness = LoadTextureAsync(metalnessPath, GL_R8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture normals = LoadTextureAsync(normalsPath, GL_RGB8, GL_REPEAT, TextureFilter::Anisotropic);

    Material material = {};
    material.type = Material::Type::PBR;
    material.workflow = Material::Workflow::Metallic;
    material.pbr.metallic.albedo = albedo;
    material.pbr.metallic.roughness = roughness;
    material.pbr.metallic.metalness = metalness;
    material.pbr.metallic.normals = normals;

    return material;
}

void CompleteMaterialLoad(Material* material) {
    if (material->type == Material::Type::PBR) {
        RendererLoadTexture(&material->pbr.metallic.albedo);
        RendererLoadTexture(&material->pbr.metallic.roughness);
        RendererLoadTexture(&material->pbr.metallic.metalness);
        RendererLoadTexture(&material->pbr.metallic.normals);
    } else {
        RendererLoadTexture(&material->legacy.diffMap);
        if (material->legacy.specMap.data) {
            RendererLoadTexture(&material->legacy.specMap);
        }
    }
}

Material LoadMaterialLegacy(const char* diffusePath, const char* specularPath) {
    Texture diffMap = LoadTexture(diffusePath, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture specMap = {};
    if (specularPath) {
        Texture specMap = LoadTexture(specularPath, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    }

    Material material = {};
    material.type = Material::Type::Legacy;
    material.legacy.diffMap = diffMap;
    material.legacy.specMap = specMap;

    return material;
}

Material LoadMaterialLegacyAsync(const char* diffusePath, const char* specularPath) {
    Texture diffMap = LoadTextureAsync(diffusePath, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture specMap = {};
    if (specularPath) {
        Texture specMap = LoadTextureAsync(specularPath, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    }

    Material material = {};
    material.type = Material::Type::Legacy;
    material.legacy.diffMap = diffMap;
    material.legacy.specMap = specMap;

    return material;
}

Material LoadMaterialPBRMetallic(const char* albedoPath, const char* roughnessPath, const char* metalnessPath, const char* normalsPath) {
    Texture albedo = LoadTexture(albedoPath, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture roughness = LoadTexture(roughnessPath, GL_R8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture metalness = LoadTexture(metalnessPath, GL_R8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture normals = LoadTexture(normalsPath, GL_RGB8, GL_REPEAT, TextureFilter::Anisotropic);


    Material material = {};
    material.type = Material::Type::PBR;
    material.workflow = Material::Workflow::Metallic;
    material.pbr.metallic.albedo = albedo;
    material.pbr.metallic.roughness = roughness;
    material.pbr.metallic.metalness = metalness;
    material.pbr.metallic.normals = normals;

    return material;
}

Material LoadMaterialPBRSpecular(const char* albedo, const char* specular, const char* gloss, const char* normals) {
    Texture albedoTex = LoadTexture(albedo, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture specularTex = LoadTexture(specular, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);
    Texture glossTex = LoadTexture(gloss, 0, GL_REPEAT, TextureFilter::Anisotropic);
    Texture normalsTex = LoadTexture(normals, GL_RGB8, GL_REPEAT, TextureFilter::Anisotropic);


    Material material = {};
    material.type = Material::Type::PBR;
    material.workflow = Material::Workflow::Specular;
    material.pbr.specular.albedo = albedoTex;
    material.pbr.specular.specular = specularTex;
    material.pbr.specular.gloss = glossTex;
    material.pbr.specular.normals = normalsTex;

    return material;
}

Texture LoadTexture(i32 width, i32 height, void* data = 0,
                    GLenum format = GL_SRGB8,
                    GLenum wrapMode = GL_REPEAT,
                    TextureFilter filter = TextureFilter::Bilinear) {
    Texture t = {};

    t.format = format;
    t.width = width;
    t.height = height;
    t.filter = filter;
    t.wrapMode = wrapMode;
    t.data = data;

    RendererLoadTexture(&t);
    assert(t.gpuHandle);

    return t;
}

const u32 AAB_FILE_MAGIC_VALUE = 0xaabaabaa;
#pragma pack(push, 1)
struct AABMeshHeaderV2 {
    u32 magicValue;
    u32 version = 2;
    u64 assetSize;
    u32 assetType;

    u32 vertexCount;
    u32 indexCount;
    u64 vertexOffset;
    u64 normalsOffset;
    u64 uvOffset;
    u64 indicesOffset;
    u64 tangentsOffset;
};
#pragma pack(pop)

Mesh* LoadMeshAAB(const wchar_t* filepath) {
    // TODO: IMPORTANT: This funtions has bugs
    // loaded mesh refers deleted memory from file
    // TODO: This is temporary
    auto mesh = (Mesh*)PlatformAlloc(sizeof(Mesh));
    *mesh = {};
    u32 fileSize = PlatformDebugGetFileSize(filepath);
    if (fileSize) {
        void* fileData = PlatformAlloc(fileSize);
        defer { PlatformFree(fileData); };
        u32 result = PlatformDebugReadFile(fileData, fileSize, filepath);
        if (result) {
            auto header = (AABMeshHeaderV2*)fileData;
            assert(header->magicValue == AAB_FILE_MAGIC_VALUE);

            mesh->vertexCount = header->vertexCount;
            mesh->indexCount = header->indexCount;

            mesh->vertices = (v3*)((byte*)fileData + header->vertexOffset);
            mesh->normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh->uvs = (v2*)((byte*)fileData + header->uvOffset);
            mesh->indices = (u32*)((byte*)fileData + header->indicesOffset);
            mesh->tangents = (v3*)((byte*)fileData + header->tangentsOffset);

            mesh->aabb = BBoxAligned::From(mesh);

            RendererLoadMesh(mesh);
            assert(mesh->gpuVertexBufferHandle);
            assert(mesh->gpuIndexBufferHandle);
        }
    }
    return mesh;
}

Mesh* LoadMeshAABAsync(const wchar_t* filepath) {
    // TODO: IMPORTANT: This garbage should be totally reworked
    // TODO: This is temporary
    auto mesh = (Mesh*)PlatformAlloc(sizeof(Mesh));
    *mesh = {};
    u32 fileSize = PlatformDebugGetFileSize(filepath);
    if (fileSize) {
        void* fileData = PlatformAlloc(fileSize);
        //defer { PlatformFree(fileData); };
        u32 result = PlatformDebugReadFile(fileData, fileSize, filepath);
        if (result) {
            auto header = (AABMeshHeaderV2*)fileData;
            assert(header->magicValue == AAB_FILE_MAGIC_VALUE);

            mesh->vertexCount = header->vertexCount;
            mesh->indexCount = header->indexCount;

            mesh->vertices = (v3*)((byte*)fileData + header->vertexOffset);
            mesh->normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh->uvs = (v2*)((byte*)fileData + header->uvOffset);
            mesh->indices = (u32*)((byte*)fileData + header->indicesOffset);
            mesh->tangents = (v3*)((byte*)fileData + header->tangentsOffset);

            mesh->aabb = BBoxAligned::From(mesh);
        }
    }
    return mesh;
}

// NOTE: Diffise only
Material LoadMaterialLegacy(i32 width, i32 height, void* bitmap) {
    Texture diffMap = LoadTexture(width, height, bitmap, GL_SRGB8, GL_REPEAT, TextureFilter::Anisotropic);

    Material material = {};
    material.type = Material::Type::Legacy;
    material.legacy.diffMap = diffMap;

    return material;
}

void GenBRDFLut(const Renderer* renderer, Texture* t) {
    assert(t->gpuHandle);
    assert(t->wrapMode == GL_CLAMP_TO_EDGE);
    assert(t->filter == TextureFilter::Bilinear);
    assert(t->format = GL_RGB16F);

    glUseProgram(renderer->shaders.BRDFIntegrator);

    glViewport(0, 0, t->width, t->height);

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, t->gpuHandle, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glFlush();
}

void ReloadShadowMaps(Renderer* renderer, u32 newResolution = 0) {
    // TODO: There are maybe could be a problems on some drivers
    // with changing framebuffer attachments so this code needs to be checked
    // on different GPUs and drivers
    if (newResolution) {
        renderer->shadowMapRes = newResolution;
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDepthTarget);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32, renderer->shadowMapRes, renderer->shadowMapRes, Renderer::NumShadowCascades, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDebugColorTarget);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, renderer->shadowMapRes, renderer->shadowMapRes, Renderer::NumShadowCascades, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

uv2 GetRenderResolution(Renderer* renderer) {
    return renderer->renderRes;
}

void ChangeRenderResolution(Renderer* renderer, uv2 newRes) {
    // TODO: There are maybe could be a problems on some drivers
    // with changing framebuffer attachments so this code needs to be checked
    // on different GPUs and drivers
    renderer->renderRes = newRes;
    glBindTexture(GL_TEXTURE_2D, renderer->offscreenColorTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, renderer->renderRes.x, renderer->renderRes.y, 0, GL_RGBA, GL_FLOAT, 0);

    glBindTexture(GL_TEXTURE_2D, renderer->offscreenDepthTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, renderer->renderRes.x, renderer->renderRes.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    glBindTexture(GL_TEXTURE_2D, renderer->srgbColorTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderer->renderRes.x, renderer->renderRes.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer* InitializeRenderer(uv2 renderRes) {
    Renderer* renderer = nullptr;
    renderer = (Renderer*)PlatformAlloc(sizeof(Renderer));
    *renderer = {};

    RecompileShaders(renderer);

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint*)&renderer->uniformBufferAligment);

    ReallocUniformBuffer(&renderer->frameUniformBuffer);
    ReallocUniformBuffer(&renderer->meshUniformBuffer);

    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_ARB, &maxAnisotropy);
    renderer->maxAnisotropy = maxAnisotropy;

    renderer->gamma = 2.4f;
    renderer->exposure = 1.0f;
    renderer->renderRes = renderRes;

    GLuint lineBufferHandle;
    glGenBuffers(1, &lineBufferHandle);
    assert(lineBufferHandle);
    renderer->lineBufferHandle = lineBufferHandle;

    glGenFramebuffers(1, &renderer->offscreenBufferHandle);
    assert(renderer->offscreenBufferHandle);
    glGenTextures(1, &renderer->offscreenColorTarget);
    assert(renderer->offscreenColorTarget);
    glGenTextures(1, &renderer->offscreenDepthTarget);
    assert(renderer->offscreenDepthTarget);

    glBindTexture(GL_TEXTURE_2D, renderer->offscreenColorTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, renderRes.x, renderRes.y, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, renderer->offscreenDepthTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, renderRes.x, renderRes.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->offscreenBufferHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->offscreenColorTarget, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->offscreenDepthTarget, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glGenFramebuffers(1, &renderer->srgbBufferHandle);
    assert(renderer->srgbBufferHandle);
    glGenTextures(1, &renderer->srgbColorTarget);
    assert(renderer->srgbColorTarget);

    glBindTexture(GL_TEXTURE_2D, renderer->srgbColorTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderRes.x, renderRes.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->srgbBufferHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->srgbColorTarget, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &renderer->captureFramebuffer);
    assert(renderer->captureFramebuffer);

    glGenFramebuffers(3, renderer->shadowMapFramebuffers);
    // TODO: Checking!
    assert(renderer->shadowMapFramebuffers[0]);

    { // Initializing depth targets
        glGenTextures(1, &renderer->shadowMapDepthTarget);
        glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDepthTarget);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32, renderer->shadowMapRes, renderer->shadowMapRes, Renderer::NumShadowCascades, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    }
    { // Initializing debug color targets
        glGenTextures(1, &renderer->shadowMapDebugColorTarget);
        glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDebugColorTarget);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, renderer->shadowMapRes, renderer->shadowMapRes, Renderer::NumShadowCascades, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    for (u32x i = 0; i < Renderer::NumShadowCascades; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadowMapFramebuffers[i]);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, renderer->shadowMapDepthTarget, 0, i);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderer->shadowMapDebugColorTarget, 0, i);
        //glDrawBuffer(GL_NONE);
        //glReadBuffer(GL_NONE);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ReloadShadowMaps(renderer);

    glGenTextures(1, &renderer->randomValuesTexture);
    assert(renderer->randomValuesTexture);
    {
        glBindTexture(GL_TEXTURE_1D, renderer->randomValuesTexture);
        defer { glBindTexture(GL_TEXTURE_1D, 0); };

        u8* randomTextureBuffer = (u8*)PlatformAlloc(sizeof(u8) * Renderer::RandomValuesTextureSize);
        defer { PlatformFree(randomTextureBuffer); };
        RandomSeries series = {};
        for (u32x i = 0; i < Renderer::RandomValuesTextureSize; i++)
        {
            randomTextureBuffer[i] = (u8)(RandomUnilateral(&series) * 255.0f);
        }
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R8, Renderer::RandomValuesTextureSize, 0, GL_RED, GL_UNSIGNED_BYTE, randomTextureBuffer);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }


    u32 brdfLUTSize = PlatformDebugGetFileSize(L"brdf_lut.aab");
    if (brdfLUTSize) {
        assert(brdfLUTSize == sizeof(f32) * 2 * 512 * 512);
        void* brdfBitmap = PlatformAlloc(brdfLUTSize);
        defer { PlatformFree(brdfBitmap); };
        auto loadedSize = PlatformDebugReadFile(brdfBitmap, brdfLUTSize, L"brdf_lut.aab");
        if (loadedSize == brdfLUTSize) {
            Texture t = LoadTexture(512, 512, brdfBitmap, GL_RG16F, GL_CLAMP_TO_EDGE, TextureFilter::Bilinear);
            if (t.gpuHandle) {
                renderer->BRDFLutHandle = t.gpuHandle;
            }
        }
    }

    if (!renderer->BRDFLutHandle) {
        Texture t = LoadTexture(512, 512, 0, GL_RG16F, GL_CLAMP_TO_EDGE, TextureFilter::Bilinear);
        assert(t.gpuHandle);
        GenBRDFLut(renderer, &t);
        renderer->BRDFLutHandle = t.gpuHandle;
        void* bitmap = PlatformAlloc(sizeof(f32) * 2 * 512 * 512);
        defer { PlatformFree(bitmap); };
        glBindTexture(GL_TEXTURE_2D, renderer->BRDFLutHandle);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, bitmap);
        PlatformDebugWriteFile(L"brdf_lut.aab", bitmap, sizeof(f32) * 2 * 512 * 512);
    }

    return renderer;
}

void GenIrradanceMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle) {
    assert(t->gpuHandle);
#if defined(PROFILE_IRRADANCE_GEN)
    SOKO_INFO("Generating irradance map...");
    glFinish();
    i64 beginTime = GetTimeStamp();
#endif

    // TODO: Make this constexpr
    static auto projInv = Inverse(PerspectiveGLRH(0.1, 10.0f, 90.0f, 1.0f)).Unwrap();
    static m3x3 capViews[] = {
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(-1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, 1.0f, 0.0f), V3(0.0f, 0.0f, 1.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, -1.0f, 0.0f), V3(0.0f, 0.0f, -1.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, 0.0f, 1.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, 0.0f, -1.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
    };

    auto prog = renderer->shaders.IrradanceConvolver;
    glUseProgram(prog);

    auto buffer = Map(renderer->frameUniformBuffer);
    buffer->invProjMatrix = projInv;
    Unmap(renderer->frameUniformBuffer);

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

    glBindTextureUnit(IrradanceConvolver::SourceCubemap, sourceHandle);

    for (u32 i = 0; i < 6; i++) {
        glViewport(0, 0, t->images[i].width, t->images[i].height);
        auto buffer = Map(renderer->frameUniformBuffer);
        buffer->invViewMatrix = M4x4(capViews[i]);
        Unmap(renderer->frameUniformBuffer);

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, t->gpuHandle, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glFlush();
#if defined (PROFILE_IRRADANCE_GEN)
    glFinish();
    i64 timeElapsed = GetTimeStamp() - beginTime;
    SOKO_INFO("Time: %i64 us", timeElapsed);
#endif

}

void GenEnvPrefiliteredMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle, u32 mipLevels) {
    assert(t->gpuHandle);
    assert(t->useMips);
    assert(t->filter == TextureFilter::Trilinear);

    const m4x4 capProj = Inverse(PerspectiveGLRH(0.1, 10.0f, 90.0f, 1.0f)).Unwrap();
    const m3x3 capViews[] = {
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(-1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, 1.0f, 0.0f), V3(0.0f, 0.0f, 1.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, -1.0f, 0.0f), V3(0.0f, 0.0f, -1.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, 0.0f, 1.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
        M3x3(Inverse(LookAtGLRH(V3(0.0f), V3(0.0f, 0.0f, -1.0f), V3(0.0f, -1.0f, 0.0f))).Unwrap()),
    };

    auto prog = renderer->shaders.EnvMapPrefilter;
    glUseProgram(prog);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

    assert(t->images[0].width == t->images[0].height);
    glUniform1i(EnvMapPrefilterShader::Resolution, t->images[0].width);

    auto buffer = Map(renderer->frameUniformBuffer);
    buffer->invProjMatrix = capProj;
    Unmap(renderer->frameUniformBuffer);

    glBindTextureUnit(EnvMapPrefilterShader::SourceCubemap, sourceHandle);

    // TODO: There are still visible seams on low mip levels

    for (u32 mipLevel = 0; mipLevel < mipLevels; mipLevel++) {
        // TODO: Pull texture size out of imges and put to a cubemap itself
        // all sides should havethe same size
        u32 w = (u32)(t->images[0].width * Pow(0.5f, (f32)mipLevel));
        u32 h = (u32)(t->images[0].height * Pow(0.5f, (f32)mipLevel));

        glViewport(0, 0, w, h);
        f32 roughness = (f32)mipLevel / (f32)(mipLevels - 1);
        glUniform1f(EnvMapPrefilterShader::Roughness, roughness);

        for (u32 i = 0; i < 6; i++) {
            // TODO: Use another buffer for this
            auto buffer = Map(renderer->frameUniformBuffer);
            buffer->invViewMatrix = M4x4(capViews[i]);
            Unmap(renderer->frameUniformBuffer);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, t->gpuHandle, mipLevel);
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    glFlush();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void DrawSkybox(Renderer* renderer, RenderGroup* group, const m4x4* invView, const m4x4* invProj) {
    glDepthMask(GL_FALSE);
    defer { glDepthMask(GL_TRUE); };

    auto prog = renderer->shaders.Skybox;
    glUseProgram(prog);

    glBindTextureUnit(SkyboxShader::CubeTexture, group->skyboxHandle);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

m3x3 MakeNormalMatrix(m4x4 model) {
    auto inverted= Inverse(model).Unwrap();
    inverted = Transpose(inverted);
    auto normal = M3x3(inverted);
    return normal;
}

// TODO: @Speed Make shure this function inlined
m4x4 CalcShadowProjection(const CameraBase* camera, f32 nearPlane, f32 farPlane, m4x4 lightLookAt, u32 shadowMapRes, bool stable) {
    // NOTE Shadow projection bounds
    v3 min;
    v3 max;

    if (stable) {
        v4 bSphereP;
        f32 bSphereR;

        // NOTE: Computing frustum bounding sphere
        // Reference: https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
        f32 ar = 1.0f / camera->aspectRatio;
        f32 k = Sqrt(1.0f + ar * ar) * Tan(ToRad(camera->fovDeg) * 0.5f);
        f32 kSq = k * k;
        f32 f = farPlane;
        f32 n = nearPlane;
        if (kSq >= ((f - n) / (f + n))) {
            bSphereP = V4(0.0f, 0.0f, -f, 1.0f);
            bSphereR = f * k;
        } else {
            bSphereP = V4(0.0f, 0.0f, -0.5f * (f + n) * (1.0f + kSq), 1.0f);
            bSphereR = 0.5f * Sqrt((f - n) * (f - n) + 2.0f * (f * f + n * n) * kSq + (f + n) * (f + n) * kSq * kSq);
        }

        // TODO: Fix this!
        // Grow sphere a little bit in order to get some safe margin
        // Because it looks like cascase boundries calculation is
        // incorrect and causes artifacts on cascade edges, especially
        // between first and second cascade
        bSphereR *= 1.3f;

        // From camera space to world space
        bSphereP = camera->invViewMatrix * bSphereP;
        // From world space to light space
        bSphereP = lightLookAt * bSphereP;

        // Constructing AABB in light space
        v3 xAxis = V3(1.0f, 0.0f, 0.0f);
        v3 yAxis = V3(0.0f, 1.0f, 0.0f);
        v3 zAxis = V3(0.0f, 0.0f, 1.0f);

        min = bSphereP.xyz - (xAxis * bSphereR + yAxis * bSphereR + zAxis * bSphereR);
        max = bSphereP.xyz + (xAxis * bSphereR + yAxis * bSphereR + zAxis * bSphereR);

        // NOTE: Update: We cannot just clamp Z because it will change AABB size and
        // destroy the stability. So here are couls be problems if Z is positive
        // which is means that near plane depth would be negative.

        // We get min and max Z values in light space, so Z is negative forward therefore if sphere
        // is in front of the light source then we have negative values.
        // Clamp max Z to 0 because if it's positive, then we are behind the light source.
        //if (max.z > 0.0f) max.z = 0.0f;

        // Inverting Z sign because we will use it for building projection matrix.
        // So it describe distance to far plane.
        min.z = -min.z;
        max.z = -max.z;
        // Swapping values so we can use min as near plane distance and max for far plane
        auto tmp = min.z;
        min.z = max.z;
        max.z = tmp;

        auto bboxSideSize = Abs(max.x - min.x);
        f32 pixelSize = bboxSideSize / shadowMapRes;

        min.x = Round(min.x / pixelSize) * pixelSize;
        min.y = Round(min.y / pixelSize) * pixelSize;
        min.z = Round(min.z / pixelSize) * pixelSize;

        max.x = Round(max.x / pixelSize) * pixelSize;
        max.y = Round(max.y / pixelSize) * pixelSize;
        max.z = Round(max.z / pixelSize) * pixelSize;

        //_OVERLAY_TRACE(min);

    } else {
        Basis cameraBasis;
        cameraBasis.zAxis = Normalize(camera->front);
        cameraBasis.xAxis = Normalize(Cross(V3(0.0f, 1.0f, 0.0), cameraBasis.zAxis));
        cameraBasis.yAxis = Cross(cameraBasis.zAxis, cameraBasis.xAxis);
        cameraBasis.p = camera->position;

        auto camFrustumCorners = GetFrustumCorners(cameraBasis, camera->fovDeg, camera->aspectRatio, nearPlane, farPlane);
        for (u32x i = 0; i < array_count(camFrustumCorners.corners); i++) {
            // TODO: Fix this!
            // Grow it a little bit in order to get some safe margin
            // Because it looks like cascase boundries calculation is
            // incorrect and causes artifacts on cascade edges, especially
            // between first and second cascade.
            camFrustumCorners.corners[i] *=  1.2f;
            camFrustumCorners.corners[i] = (lightLookAt * V4(camFrustumCorners.corners[i], 1.0f)).xyz;
        }

        min = V3(F32::Max);
        max = V3(-F32::Max);

        for (u32x i = 0; i < array_count(camFrustumCorners.corners); i++) {
            auto corner = camFrustumCorners.corners[i];
            if (corner.x < min.x) min.x = corner.x;
            if (corner.x > max.x) max.x = corner.x;
            if (corner.y < min.y) min.y = corner.y;
            if (corner.y > max.y) max.y = corner.y;
            if (corner.z < min.z) min.z = corner.z;
            if (corner.z > max.z) max.z = corner.z;
        }

        v3 minViewSpace = min;
        v3 maxViewSpace = max;

        // NOTE: Inverting Z because right-handed Z is negative-forward
        // but ortho ptojections gets constructed from Z positive-far
        auto tmp = minViewSpace.z;
        minViewSpace.z = -maxViewSpace.z;
        maxViewSpace.z = -tmp;

        min = minViewSpace;
        max = maxViewSpace;
    }

    auto result = OrthoGLRH(min.x, max.x, min.y, max.y, min.z, max.z);
    return result;
}

void RenderShadowMap(Renderer* renderer, RenderGroup* group) {
    if (group->commandQueueAt) {
        auto shader = renderer->shaders.Shadow;
        for (u32 i = 0; i < group->commandQueueAt; i++) {
            CommandQueueEntry* command = group->commandQueue + i;

            switch (command->type) {
            case RenderCommand::LineBegin:
            {
            } break;
            case RenderCommand::DrawMesh: {
                auto* data = (RenderCommandDrawMesh*)(group->renderBuffer + command->rbOffset);

                auto normalMatrix = MakeNormalMatrix(data->transform);

                auto meshBuffer = Map(renderer->meshUniformBuffer);
                meshBuffer->modelMatrix = data->transform;
                meshBuffer->normalMatrix = normalMatrix;
                Unmap(renderer->meshUniformBuffer);

                auto* mesh = data->mesh;

                while (mesh) {
                    glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                    auto posAttrLoc = ShadowPassShader::PositionAttribLocation;
                    glEnableVertexAttribArray(posAttrLoc);
                    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);
                    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
                    mesh = mesh->next;
                }
            } break;
            }
        }
    }
}

void ShadowPass(Renderer* renderer, RenderGroup* group) {
    auto light = &group->dirLight;
    auto camera = group->camera;


    glEnable(GL_POLYGON_OFFSET_FILL);
    defer { glDisable(GL_POLYGON_OFFSET_FILL); };
    glPolygonOffset(renderer->shadowSlopeBiasScale, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, (GLsizei)renderer->shadowMapRes, (GLsizei)renderer->shadowMapRes);

    auto shader = renderer->shaders.Shadow;
    glUseProgram(shader);

    for (u32x cascadeIndex = 0; cascadeIndex < Renderer::NumShadowCascades; cascadeIndex++) {
        auto viewProj = renderer->shadowCascadeViewProjMatrices[cascadeIndex];

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->shadowMapFramebuffers[cascadeIndex]);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glUniform1i(ShadowPassShader::CascadeIndexLocation, cascadeIndex);
        RenderShadowMap(renderer, group);
    }
}

void MainPass(Renderer* renderer, RenderGroup* group) {

    bool showShadowCascadesBoundaries = renderer->showShadowCascadesBoundaries;
    DEBUG_OVERLAY_TOGGLE(showShadowCascadesBoundaries);
    renderer->showShadowCascadesBoundaries = showShadowCascadesBoundaries;

    auto camera = group->camera;
    auto dirLight = group->dirLight;

    if (group->commandQueueAt) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->offscreenBufferHandle);
        glViewport(0, 0, renderer->renderRes.x, renderer->renderRes.y);
        glClearColor(renderer->clearColor.r, renderer->clearColor.g, renderer->clearColor.b, renderer->clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        for (u32 i = 0; i < group->commandQueueAt; i++) {
            CommandQueueEntry* command = group->commandQueue + i;

            switch (command->type) {
            case RenderCommand::DrawWater: {
                auto* data = (RenderCommandDrawWater*)(group->renderBuffer + command->rbOffset);
                auto program = renderer->shaders.Water;

                m3x3 normalMatrix = MakeNormalMatrix(data->transform);

                glUseProgram(program);

                auto meshBuffer = Map(renderer->meshUniformBuffer);
                meshBuffer->modelMatrix = data->transform;
                meshBuffer->normalMatrix = normalMatrix;
                Unmap(renderer->meshUniformBuffer);

                auto* mesh = data->mesh;


                glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                glEnableVertexAttribArray(WaterShader::Position);
                glEnableVertexAttribArray(WaterShader::Normal);
                glEnableVertexAttribArray(WaterShader::UV);

                u64 normalsOffset = mesh->vertexCount * sizeof(v3);
                u64 uvsOffset = normalsOffset + mesh->vertexCount * sizeof(v3);

                glVertexAttribPointer(WaterShader::Position, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glVertexAttribPointer(WaterShader::Normal, 3, GL_FLOAT, GL_FALSE, 0, (void*)normalsOffset);
                glVertexAttribPointer(WaterShader::UV, 2, GL_FLOAT, GL_FALSE, 0, (void*)uvsOffset);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);

                glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            } break;
            case RenderCommand::LineBegin: {
                auto* data = (RenderCommandLineBegin*)(group->renderBuffer + command->rbOffset);

                glUseProgram(renderer->shaders.Line);

                auto meshBuffer = Map(renderer->meshUniformBuffer);
                meshBuffer->lineColor = data->color;
                Unmap(renderer->meshUniformBuffer);

                uptr bufferSize = command->instanceCount * sizeof(RenderCommandPushLineVertex);
                void* instanceData = (void*)((byte*)data + sizeof(RenderCommandLineBegin));

                glBindBuffer(GL_ARRAY_BUFFER, renderer->lineBufferHandle);
                glBufferData(GL_ARRAY_BUFFER, bufferSize, instanceData, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(v3), 0);

                glLineWidth(data->width);

                GLuint lineType;
                switch (data->type) {
                case RenderCommandLineBegin::Segments: { lineType = GL_LINES; } break;
                case RenderCommandLineBegin::Strip: { lineType = GL_LINE_STRIP; } break;
                default: {lineType = GL_LINES; assert(false, "sdf"); } break;
                }

                glDrawArrays(lineType, 0, command->instanceCount);

            } break;
            case RenderCommand::DrawMesh: {
                auto* data = (RenderCommandDrawMesh*)(group->renderBuffer + command->rbOffset);
                if (data->material->type == Material::Type::Legacy) {
                    auto meshProg = renderer->shaders.Mesh;

                    m3x3 normalMatrix = MakeNormalMatrix(data->transform);

                    glUseProgram(meshProg);

                    auto meshBuffer = Map(renderer->meshUniformBuffer);
                    meshBuffer->modelMatrix = data->transform;
                    meshBuffer->normalMatrix = normalMatrix;
                    Unmap(renderer->meshUniformBuffer);


                    glBindTextureUnit(MeshShader::DiffMap, data->material->legacy.diffMap.gpuHandle);
                    glBindTextureUnit(MeshShader::SpecMap, data->material->legacy.specMap.gpuHandle);
                    glBindTextureUnit(MeshShader::ShadowMap, renderer->shadowMapDepthTarget);

                    auto* mesh = data->mesh;

                    while (mesh) {
                        glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                        glEnableVertexAttribArray(0);
                        glEnableVertexAttribArray(1);
                        glEnableVertexAttribArray(2);

                        u64 normalsOffset = mesh->vertexCount * sizeof(v3);
                        u64 uvsOffset = normalsOffset + mesh->vertexCount * sizeof(v3);

                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)normalsOffset);
                        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)uvsOffset);

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);

                        glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
                        mesh = mesh->next;
                    }
                } else if (data->material->type == Material::Type::PBR) {
                    assert(group->irradanceMapHandle);
                    auto meshProg = renderer->shaders.PbrMesh;
                    glUseProgram(meshProg);

                    auto* m = data->material;

                    auto normalMatrix = MakeNormalMatrix(data->transform);

                    auto meshBuffer = Map(renderer->meshUniformBuffer);
                    meshBuffer->modelMatrix = data->transform;
                    meshBuffer->normalMatrix = normalMatrix;

                    if (m->workflow == Material::Workflow::Custom) {
                        meshBuffer->metallicWorkflow = 1;
                        meshBuffer->customMaterial = 1;
                        meshBuffer->customAlbedo = m->pbr.custom.albedo;
                        meshBuffer->customRoughness = m->pbr.custom.roughness;
                        meshBuffer->customMetalness = m->pbr.custom.metalness;
                    } else {
                        meshBuffer->customMaterial = 0;
                        if (m->workflow == Material::Workflow::Metallic) {
                            meshBuffer->metallicWorkflow = 1;
                            glBindTextureUnit(MeshPBRShader::RoughnessMap, m->pbr.metallic.roughness.gpuHandle);
                            glBindTextureUnit(MeshPBRShader::MetalnessMap, m->pbr.metallic.metalness.gpuHandle);
                        } else if (m->workflow == Material::Workflow::Specular) {
                            meshBuffer->metallicWorkflow = 0;
                            glBindTextureUnit(MeshPBRShader::SpecularMap, m->pbr.specular.specular.gpuHandle);
                            glBindTextureUnit(MeshPBRShader::GlossMap, m->pbr.specular.gloss.gpuHandle);
                        } else {
                            unreachable();
                        }

                        glBindTextureUnit(MeshPBRShader::BRDFLut, renderer->BRDFLutHandle);
                        glBindTextureUnit(MeshPBRShader::AlbedoMap, m->pbr.metallic.albedo.gpuHandle);
                        glBindTextureUnit(MeshPBRShader::NormalMap, m->pbr.metallic.normals.gpuHandle);

                    }
                    Unmap(renderer->meshUniformBuffer);

                    // TODO: Are they need to be binded every shader invocation?
                    glBindTextureUnit(MeshPBRShader::IrradanceMap, group->irradanceMapHandle);
                    glBindTextureUnit(MeshPBRShader::EnviromentMap, group->envMapHandle);
                    glBindTextureUnit(MeshPBRShader::ShadowMap, renderer->shadowMapDepthTarget);

                    auto* mesh = data->mesh;

                    while (mesh) {
                        glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                        glEnableVertexAttribArray(0);
                        glEnableVertexAttribArray(1);
                        glEnableVertexAttribArray(2);
                        glEnableVertexAttribArray(3);

                        u64 normalsOffset = mesh->vertexCount * sizeof(v3);
                        u64 uvsOffset = normalsOffset + mesh->vertexCount * sizeof(v3);
                        u64 tangentsOffset = uvsOffset + mesh->vertexCount * sizeof(v2);

                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)normalsOffset);
                        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)uvsOffset);
                        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)tangentsOffset);

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);

                        glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
                        mesh = mesh->next;
                    }
                } else {
                    unreachable();
                }
            } break;

            }
        }
        Reset(group);
    }

    if (group->drawSkybox) {
        glDepthFunc(GL_EQUAL);
        DrawSkybox(renderer, group, &camera->invViewMatrix, &camera->invProjectionMatrix);
        glDepthFunc(GL_LESS);
    }
}

void Begin(Renderer* renderer, RenderGroup* group) {
    auto light = group->dirLight;
    auto camera = group->camera;

    DEBUG_OVERLAY_SLIDER(renderer->shadowSlopeBiasScale, 0.0f, 2.5f);
    DEBUG_OVERLAY_SLIDER(renderer->shadowConstantBias, 0.0f, 0.5f);


    i32 EnableStableShadows = renderer->stableShadows;
    DEBUG_OVERLAY_SLIDER(EnableStableShadows, 0, 1);
    renderer->stableShadows = EnableStableShadows;

    //
    // NOTE: Calculating light-space matrices
    //

    // TODO: Fix the mess with lookAt matrices4
    m4x4 lightLookAt = LookAtGLRH(light.from, light.from + light.dir, V3(0.0f, 1.0f, 0.0f));

    f32 frustrumDepth = camera->farPlane - camera->nearPlane;
    f32 cascadeDepth = frustrumDepth / Renderer::NumShadowCascades;
    for (u32x cascadeIndex = 0; cascadeIndex < Renderer::NumShadowCascades; cascadeIndex++) {
        f32 cascadeNear = cascadeDepth * cascadeIndex;
        f32 cascadeFar = cascadeDepth * (cascadeIndex + 1);
        renderer->shadowCascadeBounds[cascadeIndex] = camera->nearPlane + cascadeFar;
        auto proj = CalcShadowProjection(camera, cascadeNear, cascadeFar, lightLookAt, renderer->shadowMapRes, renderer->stableShadows);
        auto viewProj = proj * lightLookAt;
        renderer->shadowCascadeViewProjMatrices[cascadeIndex] = viewProj;
    }

    //
    // NOTE: Fill frame uniform buffer
    //

    m4x4 viewProj = camera->projectionMatrix * camera->viewMatrix;

    auto lightViewProj0 = renderer->shadowCascadeViewProjMatrices;
    auto lightViewProj1 = renderer->shadowCascadeViewProjMatrices + 1;
    auto lightViewProj2 = renderer->shadowCascadeViewProjMatrices + 2;

    auto frameBuffer = Map(renderer->frameUniformBuffer);
    frameBuffer->viewProjMatrix = viewProj;
    frameBuffer->viewMatrix = camera->viewMatrix;
    frameBuffer->projectionMatrix = camera->projectionMatrix;
    frameBuffer->invViewMatrix = camera->invViewMatrix;
    frameBuffer->invProjMatrix = camera->invProjectionMatrix;
    frameBuffer->lightSpaceMatrices[0] = *lightViewProj0;
    frameBuffer->lightSpaceMatrices[1] = *lightViewProj1;
    frameBuffer->lightSpaceMatrices[2] = *lightViewProj2;
    frameBuffer->dirLight.pos = light.from;
    frameBuffer->dirLight.dir = light.dir;
    frameBuffer->dirLight.ambient = light.ambient;
    frameBuffer->dirLight.diffuse = light.diffuse;
    frameBuffer->dirLight.specular = light.specular;
    frameBuffer->viewPos = camera->position;
    frameBuffer->shadowCascadeSplits = *((v3*)&renderer->shadowCascadeBounds);
    frameBuffer->showShadowCascadeBoundaries = (i32)renderer->showShadowCascadesBoundaries;
    frameBuffer->shadowFilterSampleScale = renderer->shadowFilterScale;
    frameBuffer->debugF = renderer->debugF;
    frameBuffer->debugG = renderer->debugG;
    frameBuffer->debugD = renderer->debugD;
    frameBuffer->debugNormals = renderer->debugNormals;
    frameBuffer->constShadowBias = renderer->shadowConstantBias;
    frameBuffer->gamma = renderer->gamma;
    frameBuffer->exposure = renderer->exposure;
    frameBuffer->screenSize = V2((f32)renderer->renderRes.x, (f32)renderer->renderRes.y);

    Unmap(renderer->frameUniformBuffer);
}

void End(Renderer* renderer) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->srgbBufferHandle);
    //glClearColor(renderer->clearColor.r, renderer->clearColor.g, renderer->clearColor.b, renderer->clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    auto prog = renderer->shaders.PostFx;
    glUseProgram(prog);

    glBindTextureUnit(PostFxShader::ColorSourceLinear, renderer->offscreenColorTarget);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // NOTE: FXAA pass
    static bool enableFXAA = true;
    DEBUG_OVERLAY_TOGGLE(enableFXAA);
    if (enableFXAA) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->srgbBufferHandle);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        //glClearColor(renderer->clearColor.r, renderer->clearColor.g,  renderer->clearColor.b, renderer->clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(renderer->shaders.FXAA);
        glBindTextureUnit(FXAAShader::ColorSourcePerceptual, renderer->srgbColorTarget);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->srgbBufferHandle);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, renderer->renderRes.x, renderer->renderRes.y,
                          0, 0, renderer->renderRes.x, renderer->renderRes.y,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);

    }

    static i32 showShadowMap = false;
    DEBUG_OVERLAY_SLIDER(showShadowMap, 0, 1);

    if (showShadowMap) {
        static i32 shadowCascadeLevel = 0;
        DEBUG_OVERLAY_SLIDER(shadowCascadeLevel, 0, Renderer::NumShadowCascades - 1);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->shadowMapFramebuffers[shadowCascadeLevel]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, renderer->shadowMapRes, renderer->shadowMapRes,
                          0, 0, 512, 512, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
}

#if 0
GLenum GetGLTextureFormatFromInternalFormat(GLenum internalFormat) {
    GLenum result = 0;
    switch (internalFormat) {
    case GL_SRGB8_ALPHA8: { result = GL_RGBA; } break;
    case GL_SRGB8: { result = GL_RGB; } break;
    case GL_RGB8: { result = GL_RGB; } break;
    case GL_RGB16F: { result = GL_RGB; } break;
    case GL_RG16F: { result = GL_RG; } break;
    case GL_R8: { result = GL_RED; } break;
    case GL_DEPTH_COMPONENT32F: { result = GL_DEPTH_COMPONENT; } break;
    case GL_DEPTH_COMPONENT32: { result = GL_DEPTH_COMPONENT; } break;
    case GL_DEPTH_COMPONENT24: { result = GL_DEPTH_COMPONENT; } break;
    case GL_DEPTH_COMPONENT16: { result = GL_DEPTH_COMPONENT; } break;
        invalid_default();
    }
    return result;
}

GLenum GetGLTextureTypeForInternalFormat(GLenum internalFormat) {
    GLenum type = 0;
    switch (internalFormat) {
    case GL_SRGB8_ALPHA8: { type = GL_UNSIGNED_BYTE; } break;
    case GL_SRGB8: { type = GL_UNSIGNED_BYTE; } break;
    case GL_RGB8: { type = GL_UNSIGNED_BYTE; } break;
    case GL_RGB16F: { type = GL_FLOAT; } break;
    case GL_RG16F: { type = GL_FLOAT; } break;
    case GL_R8: { type = GL_UNSIGNED_BYTE; } break;
    case GL_DEPTH_COMPONENT32F: { type = GL_FLOAT; } break;
    case GL_DEPTH_COMPONENT32: { type = GL_FLOAT; } break;
    case GL_DEPTH_COMPONENT24: { type = GL_FLOAT; } break;
    case GL_DEPTH_COMPONENT16: { type = GL_FLOAT; } break;
        invalid_default();
    }
    return type;
}

void AllocTexture2D(GLuint handle, u32x width, u32x height, u32x mipLevel, GLenum format, void* data = 0, GLenum customType = 0)
{
    GLenum secondaryFormat = GetGLTextureFormatFromInternalFormat(format);
    GLenum type;
    if (customType)
    {
        type = customType;
    }
    else
    {
        type = GetGLTextureTypeForInternalFormat(format);
    }

    glBindTexture(GL_TEXTURE_2D, handle);
    defer { glBindTexture(GL_TEXTURE_2D, 0); };

    glTexImage2D(GL_TEXTURE_2D, mipLevel, format, width, height, 0, secondaryFormat, type, data);
}
#endif
