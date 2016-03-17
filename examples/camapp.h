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

enum CamappPref
{
    camapp_pref_defaults,
    camapp_pref_speed,
    camapp_pref_resolution
};

typedef unsigned long CamappPixelFormat;
enum CamappSupportedPixelFormat
{
    camapp_pixel_format_yuyv = V4L2_PIX_FMT_YUYV
};

typedef struct
{
    void* start;
    size_t length;
} CamappBuffer;

typedef struct
{
    int fd;
    unsigned width;
    unsigned height;
    CamappPixelFormat pixel_format;
    unsigned buffers;
    CamappBuffer buffer[];
} CamappCamera;

void camapp_describe_pixel_format(CamappPixelFormat fmt, char str[5]);

CamappCamera* camapp_open_camera(char const* path, enum CamappPref pref,
                                 unsigned nbuffers);
void camapp_close_camera(CamappCamera* cam);

bool camapp_frame_ready(CamappCamera* cam);
CamappBuffer const* camapp_acquire_frame(CamappCamera* cam);
void camapp_release_frame(CamappCamera* cam, CamappBuffer const* buf);

#endif
