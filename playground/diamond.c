
#include "diamond.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static GLuint load_shader(const char *src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    if (shader)
    {
        GLint compiled;
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLchar log[1024];
            glGetShaderInfoLog(shader, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            printf("load_shader compile failed: %s\n", log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

void render_diamond(DiamondInfo* info, EGLDisplay egldisplay, EGLSurface eglsurface)
{
    EGLint width = -1;
    EGLint height = -1;

    glClear(GL_COLOR_BUFFER_BIT);
    if (eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &width) &&
        eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &height))
    {
        glViewport(0, 0, width, height);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, info->num_vertices);
}

DiamondInfo setup_diamond()
{
    const char vertex_shader_src[] =
        "attribute vec2 pos;                                \n"
        "attribute vec4 color;                              \n"
        "varying   vec4 dest_color;                         \n"
        "void main()                                        \n"
        "{                                                  \n"
        "    dest_color = color;                            \n"
        "    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);    \n"
        "}                                                  \n";
    const char fragment_shader_src[] =
        "precision mediump float;             \n"
        "varying   vec4 dest_color;           \n"
        "void main()                          \n"
        "{                                    \n"
        "    gl_FragColor = dest_color;       \n"
        "}                                    \n";
static const GLfloat vertices[] =
    {
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, -1.0f,
        -1.0f, 0.0f,
    };
static const GLfloat colors[] =
    {
        1.0f, 0.2f, 0.2f, 1.0f,
        0.2f, 1.0f, 0.2f, 1.0f,
        0.2f, 0.2f, 1.0f, 1.0f,
        0.2f, 0.2f, 0.2f, 1.0f,
    };

    GLint linked;
    DiamondInfo info;

    info.num_vertices = 4;

    info.vertex_shader = load_shader(vertex_shader_src, GL_VERTEX_SHADER);
    assert(info.vertex_shader);
    info.fragment_shader = load_shader(fragment_shader_src, GL_FRAGMENT_SHADER);
    assert(info.fragment_shader);
    info.program = glCreateProgram();
    assert(info.program);
    glAttachShader(info.program, info.vertex_shader);
    glAttachShader(info.program, info.fragment_shader);
    glLinkProgram(info.program);
    glGetProgramiv(info.program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(info.program, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        assert(-1);
    }

    glUseProgram(info.program);
    info.pos = glGetAttribLocation(info.program, "pos");
    info.color = glGetAttribLocation(info.program, "color");
    glVertexAttribPointer(info.pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(info.color, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glEnableVertexAttribArray(info.pos);
    glEnableVertexAttribArray(info.color);
    return info;
}

void destroy_diamond(DiamondInfo* info)
{
    glDisableVertexAttribArray(info->pos);
    glDisableVertexAttribArray(info->color);
    glDeleteShader(info->vertex_shader);
    glDeleteShader(info->fragment_shader);
    glDeleteProgram(info->program);
}
