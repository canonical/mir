/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 *
 */

#include "gbm_format_conversions.h"
#include <stddef.h>
#include <gbm.h>

namespace mgm = mir::graphics::mesa;

MirPixelFormat mgm::gbm_format_to_mir_format(uint32_t format)
{
    MirPixelFormat pf;

    switch (format)
    {
    case GBM_BO_FORMAT_ARGB8888:
    case GBM_FORMAT_ARGB8888:
        pf = mir_pixel_format_argb_8888;
        break;
    case GBM_BO_FORMAT_XRGB8888:
    case GBM_FORMAT_XRGB8888:
        pf = mir_pixel_format_xrgb_8888;
        break;
    case GBM_FORMAT_ABGR8888:
        pf = mir_pixel_format_abgr_8888;
        break;
    case GBM_FORMAT_XBGR8888:
        pf = mir_pixel_format_xbgr_8888;
        break;
    case GBM_FORMAT_BGR888:
        pf = mir_pixel_format_bgr_888;
        break;
    case GBM_FORMAT_RGB888:
        pf = mir_pixel_format_rgb_888;
        break;
    case GBM_FORMAT_RGB565:
        pf = mir_pixel_format_rgb_565;
        break;
    case GBM_FORMAT_RGBA5551:
        pf = mir_pixel_format_rgba_5551;
        break;
    case GBM_FORMAT_RGBA4444:
        pf = mir_pixel_format_rgba_4444;
        break;
    default:
        pf = mir_pixel_format_invalid;
        break;
    }

    return pf;
}

uint32_t mgm::mir_format_to_gbm_format(MirPixelFormat format)
{
    uint32_t gbm_pf;

    switch (format)
    {
    case mir_pixel_format_argb_8888:
        gbm_pf = GBM_FORMAT_ARGB8888;
        break;
    case mir_pixel_format_xrgb_8888:
        gbm_pf = GBM_FORMAT_XRGB8888;
        break;
    case mir_pixel_format_abgr_8888:
        gbm_pf = GBM_FORMAT_ABGR8888;
        break;
    case mir_pixel_format_xbgr_8888:
        gbm_pf = GBM_FORMAT_XBGR8888;
        break;
    case mir_pixel_format_bgr_888:
        gbm_pf = GBM_FORMAT_BGR888;
        break;
    case mir_pixel_format_rgb_888:
        gbm_pf = GBM_FORMAT_RGB888;
        break;
    case mir_pixel_format_rgb_565:
        gbm_pf = GBM_FORMAT_RGB565;
        break;
    case mir_pixel_format_rgba_5551:
        gbm_pf = GBM_FORMAT_RGBA5551;
        break;
    case mir_pixel_format_rgba_4444:
        gbm_pf = GBM_FORMAT_RGBA4444;
        break;
    default:
        gbm_pf = mgm::invalid_gbm_format;
        break;
    }

    return gbm_pf;
}
