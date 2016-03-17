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

#ifndef CAMAPP_H_
#define CAMAPP_H_

#include <linux/videodev2.h>
#include <unistd.h>
#include <stdbool.h>

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

void fourcc_string(__u32 x, char str[5]);
Camera *open_camera(const char *path, enum CameraPref pref, unsigned nbuffers);
void close_camera(Camera *cam);
bool frame_ready(Camera *cam);
const Buffer *acquire_frame(Camera *cam);
void release_frame(Camera *cam, const Buffer *buf);

#endif
