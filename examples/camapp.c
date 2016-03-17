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

#include "camapp.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>

void fourcc_string(__u32 x, char str[5])
{
    str[0] = (char)(x & 0xff);
    str[1] = (char)(x >> 8 & 0xff);
    str[2] = (char)(x >> 16 & 0xff);
    str[3] = (char)(x >> 24);
    str[4] = '\0';
}

void close_camera(Camera *cam)
{
    if (!cam) return;

    for (unsigned b = 0; b < cam->buffers; ++b)
        if (cam->buffer[b].start)
            munmap(cam->buffer[b].start, cam->buffer[b].length);
    if (cam->fd >= 0)
        close(cam->fd);
    free(cam);
}

Camera *open_camera(const char *path, enum CameraPref pref,
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

bool frame_ready(Camera *cam)
{
    struct pollfd pollfd = {cam->fd, POLLIN, 0};
    return poll(&pollfd, 1, 0) == 1 && (pollfd.revents & POLLIN);
}

const Buffer *acquire_frame(Camera *cam)
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

void release_frame(Camera *cam, const Buffer *buf)
{
    struct v4l2_buffer frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frame.memory = V4L2_MEMORY_MMAP;
    frame.index = buf - cam->buffer;
    if (ioctl(cam->fd, VIDIOC_QBUF, &frame))
        perror("VIDIOC_QBUF");
}


