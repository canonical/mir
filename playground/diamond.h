
#ifndef PLAYGROUND_DIAMOND_RENDER_H_
#define PLAYGROUND_DIAMOND_RENDER_H_
#include <GLES2/gl2.h>
#include <EGL/egl.h>

typedef struct
{
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint pos;
    GLuint color;
    GLfloat const* vertices;
    GLfloat const* colors;
    int num_vertices;
} DiamondInfo;

DiamondInfo setup_diamond();
void destroy_diamond(DiamondInfo* info);
void render_diamond(DiamondInfo* info, EGLDisplay egldisplay, EGLSurface eglsurface);

#endif /* PLAYGROUND_DIAMOND_RENDER_H_ */
