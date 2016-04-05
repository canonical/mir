/*
 * Copyright © 2016 Canonical Ltd.
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
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#define WHITE        1.0f,1.0f,1.0f,1.0f
#define DARKENED     0.0f,0.0f,0.0f,0.6f
#define BAR_TINT     WHITE
#define PREVIEW_TINT DARKENED

enum CameraPref
{
    camera_pref_defaults,
    camera_pref_speed,
    camera_pref_resolution
};

typedef unsigned long long Time;
static Time const one_second = 1000000000ULL;
static Time const one_millisecond = 1000000ULL;

typedef struct
{
    void* start;
    size_t length;
    Time timestamp;
} Buffer;

typedef struct
{
    int fd;
    struct v4l2_pix_format pix;
    unsigned buffers;
    Buffer buffer[];
} Camera;

typedef struct  // Things shared between threads
{
    pthread_mutex_t mutex;
    bool resized;
    Camera* camera;
    Time last_change_time;
    Time last_change_seen_time;
    Time display_frame_time;
    Buffer const* preview;
    int expected_direction;
} State;

static Time now()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * one_second + ts.tv_nsec;
}

static GLuint load_shader(char const* src, GLenum type)
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

static void on_event(MirSurface* surface, MirEvent const* event, void* context)
{
    (void)surface;
    State* state = (State*)context;

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

static void fourcc_string(__u32 x, char str[5])
{
    str[0] = (char)(x & 0xff);
    str[1] = (char)(x >> 8 & 0xff);
    str[2] = (char)(x >> 16 & 0xff);
    str[3] = (char)(x >> 24);
    str[4] = '\0';
}

static float interpret(Camera const* cam, Buffer const* buf)
{
    int const stride = cam->pix.bytesperline;
    int const width = cam->pix.width;
    int const height = cam->pix.height;

    int bpp = 0, middle_luminance_x = 0;
    switch (cam->pix.pixelformat)
    {
    case V4L2_PIX_FMT_YUYV:
        bpp = 2;
        middle_luminance_x = width*bpp/2;
        break;
    default:
        {
        char fmt[5];
        fourcc_string(cam->pix.pixelformat, fmt);
        fprintf(stderr, "interpret: Unsupported pixel format %s\n",
                fmt);
        }
        return NAN;
    }

    /*
     * Take a vertical line down the middle of the image, blur/smooth it
     * out, and return the y coordinate of the brightest spot...
     */
    int const radius = 20;
    int peak_start = -1, peak_end = -1;
    long max_avg = 0;
    for (int y = 0; y < height; ++y)
    {
        int from = y - radius, to = y + radius;
        if (from < 0) from = 0;
        if (to >= height) to = height - 1;

        unsigned char* p = (unsigned char*)buf->start +
                           from*stride + middle_luminance_x;
        long sum = 0;
        for (int z = from; z <= to; ++z)
        {
            sum += p[0];
            p += stride;
        }
        long avg = sum / (to - from + 1);
        if (avg > max_avg)
        {
            max_avg = avg;
            peak_start = peak_end = y;
        }
        else if (avg == max_avg && y == peak_end+1)
        {
            if (peak_start < 0) peak_start = y;
            if (y > peak_end) peak_end = y;
        }
    }
    return (peak_end + peak_start) / (2.0f * (height - 1));
}

static void close_camera(Camera* cam)
{
    if (!cam) return;

    for (unsigned b = 0; b < cam->buffers; ++b)
        if (cam->buffer[b].start)
            munmap(cam->buffer[b].start, cam->buffer[b].length);
    if (cam->fd >= 0)
        close(cam->fd);
    free(cam);
}

static Camera* open_camera(char const* path, enum CameraPref pref,
                           unsigned nbuffers)
{
    Camera* cam = calloc(1, sizeof(*cam) + nbuffers*sizeof(cam->buffer[0]));
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
    struct v4l2_pix_format* pix = &format.fmt.pix;
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
    cam->pix =* pix;

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

static Buffer const* acquire_frame(Camera* cam)
{
    struct v4l2_buffer frame;
    Buffer* buf;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam->fd, VIDIOC_DQBUF, &frame))
    {
        perror("VIDIOC_DQBUF");
        return NULL;
    }
    buf = cam->buffer + frame.index;
    buf->timestamp = frame.timestamp.tv_sec * one_second +
                     frame.timestamp.tv_usec * (one_second / 1000000);
    return buf;
}

static void release_frame(Camera const* cam, Buffer const* buf)
{
    struct v4l2_buffer frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    frame.index = buf - cam->buffer;
    if (ioctl(cam->fd, VIDIOC_QBUF, &frame))
        perror("VIDIOC_QBUF");
}

static void print_status_bar(int percent)
{
    int const max = 50;
    int len = percent * max / 100;
    printf("%3d%% [", percent);
    for (int x = 0; x < len; ++x)
        printf("o");
    len = max - len;
    for (int x = 0; x < len; ++x)
        printf(".");
    printf("]");
    fflush(stdout);
}

static void erase_line(void)
{
    printf("\r");
    for (int x = 0; x < 79; ++x)
        printf(" ");
    printf("\r");
    fflush(stdout);
}

static void* capture_thread_func(void* arg)
{
    State* state = (State*)arg;
    Camera* cam = state->camera;
    Time last_frame = now();
    int last_seen_value = -1;
    unsigned const max_history = 60;  // ~30 seconds worth of history
    unsigned nhistory = 0;
    Time history[max_history];

    printf("\n    *** Focus your camera on the screen ***\n\n");

    while (mir_eglapp_running())
    {
        Buffer const* buf = acquire_frame(cam);
        pthread_mutex_lock(&state->mutex);

        // Note using the buffer timestamp from the kernel means we're
        // free to allocate multiple camera buffers without it adversely
        // affecting the latency measurement (in fact it will improve it).
        Time acquire_time = buf->timestamp;
        Time frame_time = acquire_time - last_frame;
        last_frame = acquire_time;

        int const resolution = 5;
        int see = resolution * interpret(cam, buf);
        if ( (see != last_seen_value) &&
             (acquire_time > state->last_change_time) &&
             ( (see > last_seen_value && state->expected_direction > 0) || 
               (see < last_seen_value && state->expected_direction < 0)
             )
           )
        {
            Time latency = acquire_time - state->last_change_time;
            last_seen_value = see;
            state->last_change_seen_time = acquire_time;
            state->expected_direction = 0;

            if (latency < 10*one_second &&
                frame_time &&
                state->display_frame_time)
            {
                history[nhistory % max_history] = latency;
                ++nhistory;
                unsigned size = nhistory < max_history ? nhistory : max_history;
                Time min = ~0ULL, max = 0ULL, avg = 0;
                unsigned h = nhistory > max_history ? 0 : 1;
                for (; h < size; ++h)
                {
                    Time t = history[h];
                    avg += t;
                    if (t < min) min = t;
                    if (t > max) max = t;
                }
                avg /= size;

                // TODO: Display messages on screen in future.

                if (state->display_frame_time < 2*frame_time)
                    printf("YOUR CAMERA IS TOO SLOW. RESULTS NOT ACCURATE\n");

                Time observed_range = max - min;
                Time expected_range = frame_time + state->display_frame_time;

                erase_line();
                printf("\n");

                printf("INTERVALS: camera %llu.%1llums (%lluHz), "
                       "display %llu.%1llums (%lluHz), "
                       "expected vary %llu.%1llums\n",
                       frame_time / one_millisecond,
                       (frame_time % one_millisecond) / 100000,
                       one_second / frame_time,
                       state->display_frame_time / one_millisecond,
                       (state->display_frame_time % one_millisecond) / 100000,
                       one_second / state->display_frame_time,
                       expected_range / one_millisecond,
                       (expected_range % one_millisecond) / 100000
                       );

                printf("LATENCY: min %llu.%1llums, "
                       "max %llu.%1llums, "
                       "avg %llu.%1llums, "
                       "last %llu.%1llums, "
                       "vary %llu.%1llums\n",
                       min / one_millisecond,
                       (min % one_millisecond) / 100000,
                       max / one_millisecond,
                       (max % one_millisecond) / 100000,
                       avg / one_millisecond,
                       (avg % one_millisecond) / 100000,
                       latency / one_millisecond,
                       (latency % one_millisecond) / 100000,
                       observed_range / one_millisecond,
                       (observed_range % one_millisecond) / 100000
                       );

                if (expected_range)
                {
                    Time disparity = (Time)llabs(observed_range - expected_range);
                    printf("             ^^^^-- with an estimated error "
                           "of %llu.%1llums\n",
                           disparity / one_millisecond,
                           (disparity % one_millisecond) / 100000);
                }

                printf("COMPLETION: ");
                int percent = nhistory * 100 / (2*max_history);
                if (percent > 100) percent = 100;
                print_status_bar(percent);
            }
        }

        bool release = true;
        if (!state->preview)
        {
            state->preview = buf;
            release = false;
        }
        pthread_mutex_unlock(&state->mutex);
        if (release)
            release_frame(cam, buf);
    }
    return NULL;
}

int main(int argc, char* argv[])
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
        "uniform vec4 tint;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 f = texture2D(texture, v_texcoord);\n"
        "    gl_FragColor = vec4(tint.a * tint.rgb + (1.0-tint.a) * f.rgb,\n"
        "                        1.0);\n"
        "}\n";

    char const* const yuyv_greyscale_fshadersrc = raw_fshadersrc;
    (void)yuyv_greyscale_fshadersrc;

    // This is the Android YUV to RGB calculation.
    const char yuyv_quickcolour_fshadersrc[] =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D texture;\n"
        "uniform vec4 tint;\n"
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
        "    gl_FragColor = vec4(tint.a * tint.rgb +\n"
        "                        (1.0-tint.a) * vec3(r,g,b), 1.0);\n"
        "}\n";

    char const* const fshadersrc = yuyv_quickcolour_fshadersrc;

    Camera* cam = open_camera("/dev/video0", camera_pref_speed, 3);
    if (!cam)
    {
        fprintf(stderr, "Failed to set up camera device\n");
        return 0;  // Alan needs this to be success
    }

    // Default to fullscreen to get minimal latency (predictive bypass)
    unsigned int win_width = 0;
    unsigned int win_height = 0;
    if (!mir_eglapp_init(argc, argv, &win_width, &win_height))
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

    GLfloat const camw = cam->pix.width, camh = cam->pix.height;
    GLfloat preview[] =
    { // position   texcoord
        0.0f, camh, 0.0f, 1.0f,
        camw, camh, 1.0f, 1.0f,
        camw, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
    };
    GLint position = glGetAttribLocation(prog, "position");
    GLint texcoord = glGetAttribLocation(prog, "texcoord");
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
        true,
        cam,
        0,
        0,
        0,
        NULL,
        0
    };
    MirSurface* surface = mir_eglapp_native_surface();
    mir_surface_set_event_handler(surface, on_event, &state);

    GLint tint = glGetUniformLocation(prog, "tint");

    GLfloat bar[8] = {0,0,0,0,0,0,0,0};
    glEnableVertexAttribArray(position);
    glDisableVertexAttribArray(texcoord);

    pthread_t capture_thread;
    pthread_create(&capture_thread, NULL, capture_thread_func, &state);

    int mode = 0;
    Time last_swap_time = 0;

    while (mir_eglapp_running())
    {
        glClear(GL_COLOR_BUFFER_BIT);

        pthread_mutex_lock(&state.mutex);

        int new_mode = (2*now() / one_second) & 1;
        if (state.resized || mode != new_mode)
        {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int w = viewport[2], h = viewport[3];
            GLfloat const bar_height = 0.25f;
            GLfloat top = new_mode ? (1.0f - bar_height)*h : 0.0f;
            GLfloat bot = top + h * bar_height;
            bar[0] = 0; bar[1] = bot;
            bar[2] = w; bar[3] = bot;
            bar[4] = w; bar[5] = top;
            bar[6] = 0; bar[7] = top;

            top = h / 3.0f;
            bot = top + top;
            GLfloat left = 0.0f;
            GLfloat right = left +
                            cam->pix.width * (bot-top) / cam->pix.height;
            preview[0] =  left;  preview[1] =  bot;
            preview[4] =  right; preview[5] =  bot;
            preview[8] =  right; preview[9] =  top;
            preview[12] = left;  preview[13] = top;
            if (state.resized)
            {
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
        }
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE,
                              2*sizeof(GLfloat), bar);
        glUniform4f(tint, BAR_TINT);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        if (state.preview)
        {
            if (cam->pix.pixelformat == V4L2_PIX_FMT_YUYV)
            {
                if (fshadersrc == yuyv_greyscale_fshadersrc)
                {
                    // Greyscale, full resolution:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                                 cam->pix.width, cam->pix.height, 0,
                                 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                                 state.preview->start);
                }
                else if (fshadersrc == yuyv_quickcolour_fshadersrc)
                {
                    // Colour, half resolution:
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                 cam->pix.width/2, cam->pix.height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE,
                                 state.preview->start);
                }
            }
            else
            {
                char str[5];
                fourcc_string(cam->pix.pixelformat, str);
                fprintf(stderr, "FIXME: Unsupported camera pixel format 0x%08lx: %s\n",
                        (long)cam->pix.pixelformat, str);
            }

            release_frame(cam, state.preview);
            state.preview = NULL;
        }
        pthread_mutex_unlock(&state.mutex);

        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE,
                              4*sizeof(GLfloat), preview);
        glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE,
                              4*sizeof(GLfloat), preview+2);
        glEnableVertexAttribArray(texcoord);
        glUniform4f(tint, PREVIEW_TINT);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableVertexAttribArray(texcoord);

        if (mode != new_mode)
        {
            glFinish();
            pthread_mutex_lock(&state.mutex);
            state.last_change_time = now();
            state.expected_direction = new_mode - mode;
            pthread_mutex_unlock(&state.mutex);
            mode = new_mode;
        }
        mir_eglapp_swap_buffers();

        Time swap_time = now();
        pthread_mutex_lock(&state.mutex);
        state.display_frame_time = swap_time - last_swap_time;
        pthread_mutex_unlock(&state.mutex);
        last_swap_time = swap_time;
    }

    mir_surface_set_event_handler(surface, NULL, NULL);
    mir_eglapp_shutdown();

    pthread_join(capture_thread, NULL);
    close_camera(cam);

    return 0;
}
