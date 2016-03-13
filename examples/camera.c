/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#define _POSIX_C_SOURCE 200112L  // for setenv() from stdlib.h
#include "eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <GLES2/gl2.h>
#include <mir_toolkit/mir_surface.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef struct
{
    pthread_mutex_t mutex;
    bool resized;
} State;

typedef struct
{
    int fd;
    unsigned buffers;
    struct
    {
        void *start;
        size_t length;
    } *buffer;
    struct v4l2_pix_format pix;
} Camera;

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

static void on_event(MirSurface *surface, const MirEvent *event, void *context)
{
    (void)surface;
    State *state = (State*)context;

    // FIXME: We presently need to know that events come in on a different
    //        thread to main (LP: #1194384). When that's resolved, simple
    //        single-threaded apps like this won't need pthread.
    pthread_mutex_lock(&state->mutex);

    switch (mir_event_get_type(event))
    {
    case mir_event_type_input:
        break;
    case mir_event_type_resize:
        state->resized = true;
        break;
    case mir_event_type_close_surface:
        // TODO: eglapp.h needs a quit() function or different behaviour of
        //       mir_eglapp_shutdown().
        raise(SIGTERM);  // handled by eglapp
        break;
    default:
        break;
    }

    pthread_mutex_unlock(&state->mutex);
}

void close_camera(Camera *cam)
{
    for (unsigned b = 0; b < cam->buffers; ++b)
        munmap(cam->buffer[b].start, cam->buffer[b].length);
    free(cam->buffer);
    close(cam->fd);
}

bool open_camera(Camera *cam) /* TODO: selectable */
{
    const char *path = "/dev/video0";
    printf("Opening device: %s\n", path);
    cam->fd = open(path, O_RDWR);
    if (cam->fd < 0)
    {
        perror("open");
        return false;
    }

    struct v4l2_capability cap;
    int ret = ioctl(cam->fd, VIDIOC_QUERYCAP, &cap);
    if (ret == 0)
    {
        printf("Driver:    %s\n", cap.driver);
        printf("Card:      %s\n", cap.card);
        printf("Bus:       %s\n", cap.bus_info);
        printf("Capture:   %s\n",
            cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ? "Yes" : "No");
        printf("Streaming: %s\n",
            cap.capabilities & V4L2_CAP_STREAMING ? "Yes" : "No");

    }

    const unsigned required = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
    if (ret || (cap.capabilities & required) != required)
    {
        fprintf(stderr, "Can't get sufficient capabilities\n");
        close(cam->fd);
        return false;
    }

    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (!ioctl(cam->fd, VIDIOC_ENUM_FMT, &fmtdesc))
    {
        printf("Supports pixel format `%s' %08lx\n",
            fmtdesc.description, (long)fmtdesc.pixelformat);
        fmtdesc.index++;
    }

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(cam->fd, VIDIOC_G_FMT, &format))
    {
         perror("VIDIOC_G_FMT");
         close(cam->fd);
         return false;
    }

    struct v4l2_pix_format *pix = &format.fmt.pix;
    printf("Pixel format: %ux%u fmt %08lx, stride %u\n",
        (unsigned)pix->width, (unsigned)pix->height,
        (long)pix->pixelformat, (unsigned)pix->bytesperline);
    cam->pix = *pix;

    struct v4l2_requestbuffers req =
    {
        2,
        V4L2_BUF_TYPE_VIDEO_CAPTURE,
        V4L2_MEMORY_MMAP,
        {0,0}
    };
    if (-1 == ioctl(cam->fd, VIDIOC_REQBUFS, &req))
    {
        perror("VIDIOC_REQBUFS");
        close(cam->fd);
        return false;
    }

    cam->buffers = req.count;
    cam->buffer = calloc(cam->buffers, sizeof(*cam->buffer));
    assert(cam->buffer != NULL);
            
    for (unsigned b = 0; b < req.count; ++b)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = b;
        buf.type = req.type;
        if (-1 == ioctl(cam->fd, VIDIOC_QUERYBUF, &buf))
        {
            perror("VIDIOC_QUERYBUF");
            close(cam->fd);
            return false;
        }
        cam->buffer[b].length = buf.length;
        cam->buffer[b].start = mmap(NULL, buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                cam->fd, buf.m.offset);

        if (MAP_FAILED == cam->buffer[b].start)
        {
            perror("mmap");
            free(cam->buffer);
            close(cam->fd);
            return false;
        }
    }

    for (unsigned b = 0; b < req.count; ++b)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = b;
        buf.type = req.type;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(cam->fd, VIDIOC_QBUF, &buf))
        {
             perror("VIDIOC_QBUF");
             close_camera(cam);
             return false;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(cam->fd, VIDIOC_STREAMON, &type))
    {
        perror("VIDIOC_STREAMON");
        close_camera(cam);
        return false;
    }

    return true;
}

int acquire_frame(Camera *cam)
{
    struct v4l2_buffer frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam->fd, VIDIOC_DQBUF, &frame))
    {
        perror("Get first frame");
        free(cam->buffer);
        close(cam->fd);
        exit(-1);
    }
    return frame.index;
}

void release_frame(Camera *cam, int index)
{
    struct v4l2_buffer frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    frame.index = index;
    ioctl(cam->fd, VIDIOC_QBUF, &frame);
}

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "uniform vec2 translate;\n"
        "uniform mat4 projection;\n"
        "varying vec2 v_texcoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection *\n"
        "                  vec4(position + translate, 0.0, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    const char fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        // TODO: Implement YUYV/whatever to RGB converstion.
        //       For now we just display Y (luminance) which suffices for
        //       a greyscale image.
        "    gl_FragColor = vec4(f.r, f.g, f.b, 1.0);\n"
        "}\n";

    Camera cam;
    if (!open_camera(&cam))
    {
        fprintf(stderr, "Failed to set up camera device\n");
        return -1;
    }

    static unsigned int width = 0, height = 0;
    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    GLuint vshader = load_shader(vshadersrc, GL_VERTEX_SHADER);
    assert(vshader);
    GLuint fshader = load_shader(fshadersrc, GL_FRAGMENT_SHADER);
    assert(fshader);
    GLuint prog = glCreateProgram();
    assert(prog);
    glAttachShader(prog, vshader);
    glAttachShader(prog, fshader);
    glLinkProgram(prog);

    GLint linked;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(prog, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        return 2;
    }

    glUseProgram(prog);

    GLfloat dim = 500.0f; // TODO
    const GLfloat box[] =
    { // position      texcoord
        0.0f, dim,  0.0f, 1.0f,
        dim,  dim,  1.0f, 1.0f,
        dim,  0.0f,  1.0f, 0.0f,
        0.0f, 0.0f,  0.0f, 0.0f,
    };
    GLint position = glGetAttribLocation(prog, "position");
    GLint texcoord = glGetAttribLocation(prog, "texcoord");
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          box);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          box+2);
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(texcoord);

    GLint translate = glGetUniformLocation(prog, "translate");
    glUniform2f(translate, 0.0f, 0.0f);

    GLint projection = glGetUniformLocation(prog, "projection");

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Behave like an accumulation buffer

    State state =
    {
        PTHREAD_MUTEX_INITIALIZER,
        true
    };
    MirSurface *surface = mir_eglapp_native_surface();
    mir_surface_set_event_handler(surface, on_event, &state);

    while (mir_eglapp_running())
    {
        pthread_mutex_lock(&state.mutex);

        if (state.resized)
        {
            // mir_eglapp_swap_buffers updates the viewport for us...
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int w = viewport[2], h = viewport[3];

            // TRANSPOSED projection matrix to convert from the Mir input
            // rectangle {{0,0},{w,h}} to GL screen rectangle {{-1,1},{2,2}}.
            GLfloat matrix[16] = {2.0f/w, 0.0f,   0.0f, 0.0f,
                                  0.0f,  -2.0f/h, 0.0f, 0.0f,
                                  0.0f,   0.0f,   1.0f, 0.0f,
                                 -1.0f,   1.0f,   0.0f, 1.0f};
            // Note GL_FALSE: GLES does not support the transpose option
            glUniformMatrix4fv(projection, 1, GL_FALSE, matrix);
            state.resized = false;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        int index = acquire_frame(&cam);
        // FIXME: This is hardcoded to work with 16bpp YUYV (PlayStation Eye).
        //        It will be wrong for other cameras.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, cam.pix.width,
                     cam.pix.height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                     cam.buffer[index].start);
        release_frame(&cam, index);

        glUniform2f(translate, 0.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        pthread_mutex_unlock(&state.mutex);

        mir_eglapp_swap_buffers();
    }

    mir_surface_set_event_handler(surface, NULL, NULL);
    mir_eglapp_shutdown();
    close_camera(&cam);

    return 0;
}
