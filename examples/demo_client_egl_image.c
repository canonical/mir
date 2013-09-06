/*
 * Client-side display configuration demo.
 *
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "eglapp.h"
#include "mir_toolkit/mir_client_library.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <gbm.h>

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

struct gbm_bo* create_bo(struct gbm_device* dev, int c)
{
    struct gbm_bo* bo = gbm_bo_create(dev, 100, 100, 
                                      GBM_FORMAT_ARGB8888,
                                      GBM_BO_USE_RENDERING | GBM_BO_USE_WRITE);
    if (!bo)
    {
        fprintf(stderr, "GBM bo creation failed\n");
        return NULL;
    }

    size_t bufsize = 100 * 100 * 4;
    char *buf = (char*)malloc(100 * 100 * 4);
    memset(buf, c, bufsize);
    if (gbm_bo_write(bo, (void*)buf, bufsize))
    {
        fprintf(stderr, "GBM bo write failed\n");
        return NULL;
    }

    return bo;
}

EGLImageKHR create_image(struct gbm_bo* bo)
{
    const EGLint image_attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE
    };
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
        (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
    EGLImageKHR img = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                                        EGL_NATIVE_PIXMAP_KHR,
                                        (void*)(bo),
                                        image_attrs);
    if (img == EGL_NO_IMAGE_KHR)
        fprintf(stderr, "egl image creation failed\n");

    return img;
}

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec4 position;                               \n"
        "varying vec2 v_texcoord;                                  \n"
        "void main()                                             \n"
        "{                                                       \n"
        "    gl_Position = position;                            \n"
        "    v_texcoord = vec2(position) * vec2(0.5) + vec2(0.5); \n"
        "}                                                       \n";

    const char fshadersrc[] =
        "precision mediump float;\n"
        "uniform sampler2D tex;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "   vec4 frag = texture2D(tex, v_texcoord);\n"
        "   gl_FragColor = frag;\n"
        "}\n";

    const GLfloat vertices[] =
    {
       -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f,-1.0f,
       -1.0f,-1.0f,
    };
    GLuint vshader, fshader, prog;
    GLint linked;
    unsigned int width = 0, height = 0;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    MirConnection *connection = mir_eglapp_native_connection();
    MirPlatformPackage pkg;
    mir_connection_get_platform(connection, &pkg);
    int drm_fd = pkg.fd[0];

    struct gbm_device* gbm = gbm_create_device(drm_fd);
    if (!gbm)
    {
        fprintf(stderr, "GBM create failed\n");
        return 1;
    }

    const int num_bo = 3;
    struct gbm_bo* bos[num_bo];
    for (int i = 0; i < num_bo; i++)
        bos[i] = NULL;

    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");

    EGLImageKHR egl_images[num_bo];

    for (int i = 0; i < num_bo; i++)
        egl_images[i] = EGL_NO_IMAGE_KHR;
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    vshader = load_shader(vshadersrc, GL_VERTEX_SHADER);
    assert(vshader);
    fshader = load_shader(fshadersrc, GL_FRAGMENT_SHADER);
    assert(fshader);
    prog = glCreateProgram();
    assert(prog);
    glAttachShader(prog, vshader);
    glAttachShader(prog, fshader);
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(prog, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        return 2;
    }

    glViewport(0, 0, width, height);

    glUseProgram(prog);

    GLuint pos = glGetAttribLocation(prog, "position");

    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);
    int i = 0;

    while (mir_eglapp_running())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        int n = i % num_bo;
        if (bos[n] == NULL)
            bos[n] = create_bo(gbm, n * 80);
        if (egl_images[n] == EGL_NO_IMAGE_KHR)
            egl_images[n] = create_image(bos[n]);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_images[n]);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        mir_eglapp_swap_buffers();
        i++;
    }

    mir_eglapp_shutdown();

    return 0;
}
