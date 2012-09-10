/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mock_drm.h"
#include <gtest/gtest.h>

namespace mgg=mir::graphics::gbm;

namespace
{
mgg::MockDRM* global_mock = NULL;
}

mgg::FakeDRMResources::FakeDRMResources()
    : fd(66), resources(), crtcs(), encoders(), connectors(), mode_info(),
      crtc_ids({10, 11}), encoder_ids({20,21}), connector_ids({30, 31})
{
    resources.count_crtcs = 2;
    resources.crtcs = crtc_ids;
    resources.count_connectors = 2;
    resources.connectors = connector_ids;
    resources.count_encoders = 2;
    resources.encoders = encoder_ids;

    crtcs[0].crtc_id = 10;
    crtcs[1].crtc_id = 11;

    encoders[0].encoder_id = 20;
    encoders[1].encoder_id = 21;
    encoders[1].crtc_id = 11;

    connectors[0].connector_id = 30;
    connectors[0].connection = DRM_MODE_DISCONNECTED;
    connectors[0].subpixel = DRM_MODE_SUBPIXEL_UNKNOWN;
    connectors[1].connector_id = 31;
    connectors[1].encoder_id = 21;
    connectors[1].connection = DRM_MODE_CONNECTED;
    connectors[1].subpixel = DRM_MODE_SUBPIXEL_UNKNOWN;
    connectors[1].count_modes = 1;
    connectors[1].modes = &mode_info;
}

mgg::MockDRM::MockDRM()
{
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, drmOpen(_,_))
    .WillByDefault(Return(fake_drm.fd));

    ON_CALL(*this, drmModeGetResources(fake_drm.fd))
    .WillByDefault(Return(&fake_drm.resources));

    ON_CALL(*this, drmModeGetCrtc(fake_drm.fd, 10))
    .WillByDefault(Return(&fake_drm.crtcs[0]));
    ON_CALL(*this, drmModeGetCrtc(fake_drm.fd, 11))
    .WillByDefault(Return(&fake_drm.crtcs[1]));

    ON_CALL(*this, drmModeGetEncoder(fake_drm.fd, 20))
    .WillByDefault(Return(&fake_drm.encoders[0]));
    ON_CALL(*this, drmModeGetEncoder(fake_drm.fd, 21))
    .WillByDefault(Return(&fake_drm.encoders[1]));

    ON_CALL(*this, drmModeGetConnector(fake_drm.fd, 30))
    .WillByDefault(Return(&fake_drm.connectors[0]));
    ON_CALL(*this, drmModeGetConnector(fake_drm.fd, 31))
    .WillByDefault(Return(&fake_drm.connectors[1]));
}

mgg::MockDRM::~MockDRM()
{
    global_mock = NULL;
}

int drmOpen(const char *name, const char *busid)
{
    return global_mock->drmOpen(name, busid);
}

int drmClose(int fd)
{
    return global_mock->drmClose(fd);
}

drmModeResPtr drmModeGetResources(int fd)
{
    return global_mock->drmModeGetResources(fd);
}

drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connectorId)
{
    return global_mock->drmModeGetConnector(fd, connectorId);
}

drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id)
{
    return global_mock->drmModeGetEncoder(fd, encoder_id);
}

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId)
{
    return global_mock->drmModeGetCrtc(fd, crtcId);
}

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId,
                   uint32_t x, uint32_t y, uint32_t *connectors, int count,
                   drmModeModeInfoPtr mode)
{
    return global_mock->drmModeSetCrtc(fd, crtcId, bufferId, x, y,
                                       connectors, count, mode);
}

void drmModeFreeResources(drmModeResPtr ptr)
{
    global_mock->drmModeFreeResources(ptr);
}

void drmModeFreeConnector(drmModeConnectorPtr ptr)
{
    global_mock->drmModeFreeConnector(ptr);
}

void drmModeFreeEncoder(drmModeEncoderPtr ptr)
{
    global_mock->drmModeFreeEncoder(ptr);
}

void drmModeFreeCrtc(drmModeCrtcPtr ptr)
{
    global_mock->drmModeFreeCrtc(ptr);
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height,
                 uint8_t depth, uint8_t bpp, uint32_t pitch,
                 uint32_t bo_handle, uint32_t *buf_id)
{

    return global_mock->drmModeAddFB(fd, width, height, depth, bpp,
                                     pitch, bo_handle, buf_id);
}

int drmModeRmFB(int fd, uint32_t bufferId)
{
    return global_mock->drmModeRmFB(fd, bufferId);
}


int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id,
                    uint32_t flags, void *user_data)
{
    return global_mock->drmModePageFlip(fd, crtc_id, fb_id,
                                        flags, user_data);
}

int drmHandleEvent(int fd, drmEventContextPtr evctx)
{
    return global_mock->drmHandleEvent(fd, evctx);
}
