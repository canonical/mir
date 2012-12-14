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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_MIR_TEST_GRAPHICS_GBM_MOCK_DRM_H_
#define MIR_MIR_TEST_GRAPHICS_GBM_MOCK_DRM_H_

#include <gmock/gmock.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace gbm
{

class FakeDRMResources
{
public:
    FakeDRMResources();
    ~FakeDRMResources();

    int fd;
    drmModeRes resources;
    drmModeCrtc crtcs[2];
    drmModeEncoder encoders[2];
    drmModeConnector connectors[2];
    drmModeModeInfo mode_info;
    uint32_t crtc_ids[2];
    uint32_t encoder_ids[2];
    uint32_t connector_ids[2];

private:
    int pipe_fds[2];
};

class MockDRM
{
public:
    MockDRM();
    ~MockDRM();

    MOCK_METHOD2(drmOpen, int(const char *name, const char *busid));
    MOCK_METHOD1(drmClose, int(int fd));
    MOCK_METHOD3(drmIoctl, int(int fd, unsigned long request, void *arg));

    MOCK_METHOD1(drmModeGetResources, drmModeResPtr(int fd));
    MOCK_METHOD2(drmModeGetConnector, drmModeConnectorPtr(int fd, uint32_t connectorId));
    MOCK_METHOD2(drmModeGetEncoder, drmModeEncoderPtr(int fd, uint32_t encoder_id));
    MOCK_METHOD2(drmModeGetCrtc, drmModeCrtcPtr(int fd, uint32_t crtcId));
    MOCK_METHOD8(drmModeSetCrtc, int(int fd, uint32_t crtcId, uint32_t bufferId,
                                     uint32_t x, uint32_t y, uint32_t *connectors,
                                     int count, drmModeModeInfoPtr mode));

    MOCK_METHOD1(drmModeFreeResources, void(drmModeResPtr ptr));
    MOCK_METHOD1(drmModeFreeConnector, void(drmModeConnectorPtr ptr));
    MOCK_METHOD1(drmModeFreeEncoder, void(drmModeEncoderPtr ptr));
    MOCK_METHOD1(drmModeFreeCrtc, void(drmModeCrtcPtr ptr));

    MOCK_METHOD8(drmModeAddFB, int(int fd, uint32_t width, uint32_t height,
                                   uint8_t depth, uint8_t bpp, uint32_t pitch,
                                   uint32_t bo_handle, uint32_t *buf_id));
    MOCK_METHOD2(drmModeRmFB, int(int fd, uint32_t bufferId));

    MOCK_METHOD5(drmModePageFlip, int(int fd, uint32_t crtc_id, uint32_t fb_id,
                                                  uint32_t flags, void *user_data));
    MOCK_METHOD2(drmHandleEvent, int(int fd, drmEventContextPtr evctx));

    MOCK_METHOD2(drmGetMagic, int(int fd, drm_magic_t *magic));
    MOCK_METHOD2(drmAuthMagic, int(int fd, drm_magic_t magic));

    FakeDRMResources fake_drm;
};

}
}
}

#endif /* MIR_MIR_TEST_GRAPHICS_GBM_DRM_MOCK_H_ */
