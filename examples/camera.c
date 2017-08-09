/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir_toolkit/mir_client_library.h"
#include "eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <GLES2/gl2.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

typedef struct
{
    pthread_mutex_t mutex;
    bool resized;
} State;

enum CameraPref
{
    camera_pref_defaults,
    camera_pref_speed,
    camera_pref_resolution
};

typedef struct
{
    void *start;
    size_t length;
} Buffer;

typedef struct
{
    int fd;
    struct v4l2_pix_format pix;
    unsigned buffers;
    Buffer buffer[];
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

static void on_event(MirWindow *surface, const MirEvent *event, void *context)
{
    State *state = (State*)context;

    // FIXME: We presently need to know that events come in on a different
    //        thread to main (LP: #1194384). When that's resolved, simple
    //        single-threaded apps like this won't need pthread.
    pthread_mutex_lock(&state->mutex);

    switch (mir_event_get_type(event))
    {
    case mir_event_type_resize:
        state->resized = true;
        break;
    default:
        break;
    }

    pthread_mutex_unlock(&state->mutex);

    mir_eglapp_handle_event(surface, event, NULL);
}

static void fourcc_string(__u32 x, char str[5])
{
    str[0] = (char)(x & 0xff);
    str[1] = (char)(x >> 8 & 0xff);
    str[2] = (char)(x >> 16 & 0xff);
    str[3] = (char)(x >> 24);
    str[4] = '\0';
}

static void close_camera(Camera *cam)
{
    if (!cam) return;

    for (unsigned b = 0; b < cam->buffers; ++b)
        if (cam->buffer[b].start)
            munmap(cam->buffer[b].start, cam->buffer[b].length);
    if (cam->fd >= 0)
        close(cam->fd);
    free(cam);
}

static Camera *open_camera(const char *path, enum CameraPref pref,
                           unsigned nbuffers)
{
    Camera *cam = calloc(1, sizeof(*cam) + nbuffers*sizeof(cam->buffer[0]));
    if (cam == NULL)
    {
        perror("malloc");
        goto fail;
    }

    printf("Opening device: %s\n", path);
    cam->fd = open(path, O_RDWR);
    if (cam->fd < 0)
    {
        perror("open");
        goto fail;
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
        goto fail;
    }

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_pix_format *pix = &format.fmt.pix;
    // Driver will choose the best match
    if (pref == camera_pref_speed)
    {
        pix->width = 1;
        pix->height = 1;
    }
    else if (pref == camera_pref_resolution)
    {
        pix->width = 9999;
        pix->height = 9999;
    }
    // But we really only need it to honour these:
    pix->pixelformat = V4L2_PIX_FMT_YUYV;
    pix->field = V4L2_FIELD_NONE;
    // Just try, best effort. This may fail.
    if (ioctl(cam->fd, VIDIOC_S_FMT, &format) &&
        ioctl(cam->fd, VIDIOC_G_FMT, &format))
    {
        perror("VIDIOC_[SG]_FMT");
        goto fail;
    }
    char str[5];
    fourcc_string(pix->pixelformat, str);
    printf("Pixel format: %ux%u fmt %s, stride %u\n",
        (unsigned)pix->width, (unsigned)pix->height,
        str, (unsigned)pix->bytesperline);
    cam->pix = *pix;

    // Always choose the highest frame rate. Although what you will get
    // depends on the resolution vs speed set above.
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 1000;
    if (ioctl(cam->fd, VIDIOC_S_PARM, &parm))
    {
        fprintf(stderr, "Setting frame rate is not supported.\n");
    }
    else
    {
        unsigned hz = parm.parm.capture.timeperframe.denominator /
                      parm.parm.capture.timeperframe.numerator;
        printf("Maximum frame rate requested: %u Hz (may be less)\n", hz);
    }

    struct v4l2_requestbuffers req =
    {
        nbuffers,
        V4L2_BUF_TYPE_VIDEO_CAPTURE,
        V4L2_MEMORY_MMAP,
        {0,0}
    };
    if (-1 == ioctl(cam->fd, VIDIOC_REQBUFS, &req))
    {
        perror("VIDIOC_REQBUFS");
        goto fail;
    }

    cam->buffers = req.count;

    for (unsigned b = 0; b < req.count; ++b)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = b;
        buf.type = req.type;
        if (-1 == ioctl(cam->fd, VIDIOC_QUERYBUF, &buf))
        {
            perror("VIDIOC_QUERYBUF");
            goto fail;
        }
        cam->buffer[b].length = buf.length;
        cam->buffer[b].start = mmap(NULL, buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                cam->fd, buf.m.offset);

        if (MAP_FAILED == cam->buffer[b].start)
        {
            perror("mmap");
            goto fail;
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
            goto fail;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(cam->fd, VIDIOC_STREAMON, &type))
    {
        perror("VIDIOC_STREAMON");
        goto fail;
    }

    return cam;
fail:
    close_camera(cam);
    return NULL;
}

static bool frame_ready(Camera *cam)
{
    struct pollfd pollfd = {cam->fd, POLLIN, 0};
    return poll(&pollfd, 1, 0) == 1 && (pollfd.revents & POLLIN);
}

static const Buffer *acquire_frame(Camera *cam)
{
    struct v4l2_buffer frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam->fd, VIDIOC_DQBUF, &frame))
    {
        perror("VIDIOC_DQBUF");
        return NULL;
    }
    return cam->buffer + frame.index;
}

static void release_frame(Camera *cam, const Buffer *buf)
{
    struct v4l2_buffer frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    frame.index = buf - cam->buffer;
    if (ioctl(cam->fd, VIDIOC_QBUF, &frame))
        perror("VIDIOC_QBUF");
}

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "uniform mat4 projection;\n"
        "varying vec2 v_texcoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection *\n"
        "                  vec4(position, 0.0, 1.0);\n"
        "    v_texcoord = texcoord;\n"
        "}\n";

    const char raw_fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        "    gl_FragColor = vec4(f.rgb, 1.0);\n"
        "}\n";

    const char * const yuyv_greyscale_fshadersrc = raw_fshadersrc;

    // This is the Android YUV to RGB calculation.
    // TODO: Vary the shader to match the camera's reported colour space
    const char yuyv_quickcolour_fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        "    float y = (f.r + f.b) / 2.0;\n"  // Y unsigned (from two pixels)
        "    float u = f.g - 0.5;\n"       // U signed (same for both pixels)
        "    float v = f.a - 0.5;\n"       // V signed (same for both pixels)
        "    float r = clamp(y + 1.370705*v, 0.0, 1.0);\n"
        "    float g = clamp(y - 0.698001*v - 0.337633*u, 0.0, 1.0);\n"
        "    float b = clamp(y + 1.732446*u, 0.0, 1.0);\n"
        "    gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";

    // TODO: Selectable between high-res grey vs half-res colour?
    const char * const fshadersrc = yuyv_quickcolour_fshadersrc;

    unsigned int win_width = 1;
    unsigned int win_height = 1;

    char const* dev_video = "/dev/video0";
    mir_eglapp_bool ultrafast = 0;
    struct mir_eglapp_arg custom_args[] =
    {
        {"-d <path>", "=", &dev_video, "Path to camera device"},
        {"-u", "!", &ultrafast, "Ultra fast mode (low resolution)"},
        {NULL, NULL, NULL, NULL},
    };
    if (!mir_eglapp_init(argc, argv, &win_width, &win_height, custom_args))
        return 1;

    // By default we prefer high resolution and low CPU usage but if you
    // ask for ultrafast mode expect low resolution and high CPU usage...
    enum CameraPref pref = camera_pref_resolution;
    if (ultrafast)
    {
        pref = camera_pref_speed;
//        MirBufferStream* bs = mir_window_get_buffer_stream(mir_eglapp_native_window());
//        mir_buffer_stream_set_swapinterval(bs, 0);
    }
    Camera *cam = open_camera(dev_video, pref, 1);
    if (!cam)
    {
        fprintf(stderr, "Failed to set up camera device\n");
        return 0;
    }

    MirWindow* window = mir_eglapp_native_window();
    if (win_width == 1)  /* Fullscreen was not chosen */
    {
        /* Chicken or egg? init before open_camera, before size is known */
        MirConnection* connection = mir_eglapp_native_connection();
        MirWindowSpec* changes = mir_create_window_spec(connection);
        win_width = cam->pix.width;
        win_height = cam->pix.height;
        mir_window_spec_set_width(changes, win_width);
        mir_window_spec_set_height(changes, win_height);
        mir_window_apply_spec(window, changes);
        mir_window_spec_release(changes);
    }

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

    const GLfloat camw = cam->pix.width, camh = cam->pix.height;
    const GLfloat box[] =
    { // position   texcoord
        0.0f, camh, 0.0f, 1.0f,
        camw, camh, 1.0f, 1.0f,
        camw, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
    };
    GLint position = glGetAttribLocation(prog, "position");
    GLint texcoord = glGetAttribLocation(prog, "texcoord");
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          box);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
                          box+2);
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(texcoord);

    GLint projection = glGetUniformLocation(prog, "projection");

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, win_width, win_height);

    State state =
    {
        PTHREAD_MUTEX_INITIALIZER,
        true
    };
    mir_window_set_event_handler(window, on_event, &state);

    bool first_frame = true;
    while (mir_eglapp_running())
    {
        bool wait_for_new_frame = true;
        pthread_mutex_lock(&state.mutex);

        if (state.resized)
        {
            // mir_eglapp_swap_buffers updates the viewport for us...
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            GLfloat scalex = 2.0f / viewport[2];
            GLfloat scaley = -2.0f / viewport[3];

            // Expand image to fit:
            GLfloat scalew = (GLfloat)viewport[2] / cam->pix.width;
            GLfloat scaleh = (GLfloat)viewport[3] / cam->pix.height;

            GLfloat scale;
            GLfloat offsetx = -1.0f, offsety = 1.0f;
            if (scalew <= scaleh)
            {
                scale = scalew;
                offsety -= (GLfloat)(viewport[3] - scale*cam->pix.height) /
                           viewport[3];
            }
            else
            {
                scale = scaleh;
                offsetx += (GLfloat)(viewport[2] - scale*cam->pix.width) /
                           viewport[2];
            }

            scalex *= scale;
            scaley *= scale;

            // TRANSPOSED projection matrix to convert from the Mir input
            // rectangle {{0,0},{w,h}} to GL screen rectangle {{-1,1},{2,2}}.
            GLfloat matrix[16] = {scalex, 0.0f,   0.0f, 0.0f,
                                  0.0f,   scaley, 0.0f, 0.0f,
                                  0.0f,   0.0f,   1.0f, 0.0f,
                                  offsetx,offsety,0.0f, 1.0f};

            // Note GL_FALSE: GLES does not support the transpose option
            glUniformMatrix4fv(projection, 1, GL_FALSE, matrix);
            state.resized = false;
            wait_for_new_frame = first_frame;
            first_frame = false;
        }

        if (wait_for_new_frame || frame_ready(cam))
        {
            const Buffer *buf = acquire_frame(cam);
            if (cam->pix.pixelformat == V4L2_PIX_FMT_YUYV)
            {
                if (fshadersrc == yuyv_greyscale_fshadersrc)
                {
                    // Greyscale, full resolution:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                                 cam->pix.width, cam->pix.height, 0,
                                 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                                 buf->start);
                }
                else if (fshadersrc == yuyv_quickcolour_fshadersrc)
                {
                    // Colour, half resolution:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                 cam->pix.width/2, cam->pix.height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE,
                                 buf->start);
                }
                // TODO: Colour, full resolution. But it will be slow :(
            }
            else
            {
                char str[5];
                fourcc_string(cam->pix.pixelformat, str);
                fprintf(stderr, "FIXME: Unsupported camera pixel format 0x%08lx: %s\n",
                        (long)cam->pix.pixelformat, str);
            }
            release_frame(cam, buf);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        pthread_mutex_unlock(&state.mutex);

        mir_eglapp_swap_buffers();
    }

    mir_window_set_event_handler(window, NULL, NULL);
    mir_eglapp_cleanup();
    close_camera(cam);

    return 0;
}
