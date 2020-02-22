#pragma once

struct Context {
    GLuint prog;
    GLuint vbo;
};

void FluxInit(Context* context);
void FluxReload(Context* context);
void FluxUpdate(Context* context);
void FluxRender(Context* context);
