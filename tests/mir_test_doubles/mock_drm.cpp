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

#include "mir/test/doubles/mock_drm.h"
#include "mir/geometry/size.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include <unistd.h>
#include <dlfcn.h>

namespace mtd=mir::test::doubles;
namespace geom = mir::geometry;

namespace
{
mtd::MockDRM* global_mock = nullptr;
}

mtd::FakeDRMResources::FakeDRMResources()
    : pipe_fds{-1, -1}
{
    /* Use the read end of a pipe as the fake DRM fd */
    if (pipe(pipe_fds) < 0 || pipe_fds[0] < 0)
        throw std::runtime_error("Failed to create fake DRM fd");

    /* Add some default resources */
    uint32_t const invalid_id{0};
    uint32_t const crtc0_id{10};
    uint32_t const crtc1_id{11};
    uint32_t const encoder0_id{20};
    uint32_t const encoder1_id{21};
    uint32_t const connector0_id{30};
    uint32_t const connector1_id{31};
    uint32_t const all_crtcs_mask{0x3};

    modes.push_back(create_mode(1920, 1080, 138500, 2080, 1111, PreferredMode));
    modes.push_back(create_mode(832, 624, 57284, 1152, 667, NormalMode));
    connector_encoder_ids.push_back(encoder0_id);
    connector_encoder_ids.push_back(encoder1_id);

    add_crtc(crtc0_id, drmModeModeInfo());
    add_crtc(crtc1_id, modes[1]);

    add_encoder(encoder0_id, invalid_id, all_crtcs_mask);
    add_encoder(encoder1_id, crtc1_id, all_crtcs_mask);

    add_connector(connector0_id, DRM_MODE_CONNECTOR_VGA,
                  DRM_MODE_DISCONNECTED, invalid_id,
                  modes_empty, connector_encoder_ids, geom::Size());
    add_connector(connector1_id, DRM_MODE_CONNECTOR_DVID,
                  DRM_MODE_CONNECTED, encoder1_id,
                  modes, connector_encoder_ids,
                  geom::Size{121, 144});

    prepare();
}

mtd::FakeDRMResources::~FakeDRMResources()
{
    if (pipe_fds[0] >= 0)
        close(pipe_fds[0]);

    if (pipe_fds[1] >= 0)
        close(pipe_fds[1]);
}

int mtd::FakeDRMResources::fd() const
{
    return pipe_fds[0];
}

int mtd::FakeDRMResources::write_fd() const
{
    return pipe_fds[1];
}

drmModeRes* mtd::FakeDRMResources::resources_ptr()
{
    return &resources;
}

void mtd::FakeDRMResources::prepare()
{
    resources.count_crtcs = crtcs.size();
    for (auto const& crtc: crtcs)
        crtc_ids.push_back(crtc.crtc_id);
    resources.crtcs = crtc_ids.data();

    resources.count_encoders = encoders.size();
    for (auto const& encoder: encoders)
        encoder_ids.push_back(encoder.encoder_id);
    resources.encoders = encoder_ids.data();

    resources.count_connectors = connectors.size();
    for (auto const& connector: connectors)
        connector_ids.push_back(connector.connector_id);
    resources.connectors = connector_ids.data();
}

void mtd::FakeDRMResources::reset()
{
    resources = drmModeRes();

    crtcs.clear();
    encoders.clear();
    connectors.clear();

    crtc_ids.clear();
    encoder_ids.clear();
    connector_ids.clear();
}

void mtd::FakeDRMResources::add_crtc(uint32_t id, drmModeModeInfo mode)
{
    drmModeCrtc crtc = drmModeCrtc();

    crtc.crtc_id = id;
    crtc.mode = mode;

    crtcs.push_back(crtc);
}

void mtd::FakeDRMResources::add_encoder(uint32_t encoder_id, uint32_t crtc_id,
                                        uint32_t possible_crtcs_mask)
{
    drmModeEncoder encoder = drmModeEncoder();

    encoder.encoder_id = encoder_id;
    encoder.crtc_id = crtc_id;
    encoder.possible_crtcs = possible_crtcs_mask;

    encoders.push_back(encoder);
}

void mtd::FakeDRMResources::add_connector(uint32_t connector_id,
                                          uint32_t type,
                                          drmModeConnection connection,
                                          uint32_t encoder_id,
                                          std::vector<drmModeModeInfo>& modes,
                                          std::vector<uint32_t>& possible_encoder_ids,
                                          geom::Size const& physical_size)
{
    drmModeConnector connector = drmModeConnector();

    connector.connector_id = connector_id;
    connector.connector_type = type;
    connector.connection = connection;
    connector.encoder_id = encoder_id;
    connector.modes = modes.data();
    connector.count_modes = modes.size();
    connector.encoders = possible_encoder_ids.data();
    connector.count_encoders = possible_encoder_ids.size();
    connector.mmWidth = physical_size.width.as_uint32_t();
    connector.mmHeight = physical_size.height.as_uint32_t();

    connectors.push_back(connector);
}

drmModeCrtc* mtd::FakeDRMResources::find_crtc(uint32_t id)
{
    for (auto& crtc : crtcs)
    {
        if (crtc.crtc_id == id)
            return &crtc;
    }
    return nullptr;
}

drmModeEncoder* mtd::FakeDRMResources::find_encoder(uint32_t id)
{
    for (auto& encoder : encoders)
    {
        if (encoder.encoder_id == id)
            return &encoder;
    }
    return nullptr;
}

drmModeConnector* mtd::FakeDRMResources::find_connector(uint32_t id)
{
    for (auto& connector : connectors)
    {
        if (connector.connector_id == id)
            return &connector;
    }
    return nullptr;
}


drmModeModeInfo mtd::FakeDRMResources::create_mode(uint16_t hdisplay, uint16_t vdisplay,
                                                   uint32_t clock, uint16_t htotal,
                                                   uint16_t vtotal,
                                                   ModePreference preferred)
{
    drmModeModeInfo mode = drmModeModeInfo();

    mode.hdisplay = hdisplay;
    mode.vdisplay = vdisplay;
    mode.clock = clock;
    mode.htotal = htotal;
    mode.vtotal = vtotal;

    uint32_t total = htotal;
    total *= vtotal;  // extend to 32 bits
    mode.vrefresh = clock * 1000UL / total;

    if (preferred)
        mode.type |= DRM_MODE_TYPE_PREFERRED;

    return mode;
}

mtd::MockDRM::MockDRM()
{
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, open(_,_,_))
    .WillByDefault(Return(fake_drm.fd()));

    ON_CALL(*this, drmOpen(_,_))
    .WillByDefault(Return(fake_drm.fd()));

    ON_CALL(*this, drmModeGetResources(_))
    .WillByDefault(Return(fake_drm.resources_ptr()));

    ON_CALL(*this, drmModeGetCrtc(_, _))
    .WillByDefault(WithArgs<1>(Invoke(&fake_drm, &FakeDRMResources::find_crtc)));

    ON_CALL(*this, drmModeGetEncoder(_, _))
    .WillByDefault(WithArgs<1>(Invoke(&fake_drm, &FakeDRMResources::find_encoder)));

    ON_CALL(*this, drmModeGetConnector(_, _))
    .WillByDefault(WithArgs<1>(Invoke(&fake_drm, &FakeDRMResources::find_connector)));

    ON_CALL(*this, drmSetInterfaceVersion(_, _))
    .WillByDefault(Return(0));

    ON_CALL(*this, drmGetBusid(_))
    .WillByDefault(WithoutArgs(Invoke([]{ return static_cast<char*>(malloc(10)); })));
}

mtd::MockDRM::~MockDRM() noexcept
{
    global_mock = nullptr;
}

int drmOpen(const char *name, const char *busid)
{
    return global_mock->drmOpen(name, busid);
}

int drmClose(int fd)
{
    return global_mock->drmClose(fd);
}

int drmIoctl(int fd, unsigned long request, void *arg)
{
    return global_mock->drmIoctl(fd, request, arg);
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

int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, uint32_t bo_handles[4],
                  uint32_t pitches[4], uint32_t offsets[4],
                  uint32_t *buf_id, uint32_t flags)
{
    return global_mock->drmModeAddFB2(fd, width, height, pixel_format,
                                      bo_handles, pitches, offsets,
                                      buf_id, flags);
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

int drmGetMagic(int fd, drm_magic_t *magic)
{
    return global_mock->drmGetMagic(fd, magic);
}

int drmAuthMagic(int fd, drm_magic_t magic)
{
    return global_mock->drmAuthMagic(fd, magic);
}

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    return global_mock->drmPrimeHandleToFD(fd, handle, flags, prime_fd);
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
    return global_mock->drmPrimeFDToHandle(fd, prime_fd, handle);
}

int drmSetMaster(int fd)
{
    return global_mock->drmSetMaster(fd);
}

int drmDropMaster(int fd)
{
    return global_mock->drmDropMaster(fd);
}

int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle, uint32_t width, uint32_t height)
{
    return global_mock->drmModeSetCursor(fd, crtcId, bo_handle, width, height);
}

int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y)
{
    return global_mock->drmModeMoveCursor(fd, crtcId, x, y);
}

int drmSetInterfaceVersion(int fd, drmSetVersion* sv)
{
    return global_mock->drmSetInterfaceVersion(fd, sv);
}

char* drmGetBusid(int fd)
{
    return global_mock->drmGetBusid(fd);
}

// We need to wrap open as we sometimes open() the DRM device
// We need to explicitly mark this as C because we don't match the
// libc header; we only care about the three-parameter version
extern "C"
{
int open(char const* path, int flags, mode_t mode)
{
    char const* drm_prefix = "/dev/dri/";
    if (!strncmp(path, drm_prefix, strlen(drm_prefix)))
        return global_mock->open(path, flags, mode);

    int (*real_open)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "open");

    return (*real_open)(path, flags, mode);
}

int open64(char const* path, int flags, mode_t mode)
{
    char const* drm_prefix = "/dev/dri/";
    if (!strncmp(path, drm_prefix, strlen(drm_prefix)))
        return global_mock->open(path, flags, mode);

    int (*real_open64)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "open64");

    return (*real_open64)(path, flags, mode);
}

int __open(char const* path, int flags, mode_t mode)
{
    char const* drm_prefix = "/dev/dri/";
    if (!strncmp(path, drm_prefix, strlen(drm_prefix)))
        return global_mock->open(path, flags, mode);

    int (*real_open)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open) = dlsym(RTLD_NEXT, "__open");

    return (*real_open)(path, flags, mode);
}

int __open64(char const* path, int flags, mode_t mode)
{
    char const* drm_prefix = "/dev/dri/";
    if (!strncmp(path, drm_prefix, strlen(drm_prefix)))
        return global_mock->open(path, flags, mode);

    int (*real_open64)(char const *path, int flags, mode_t mode);
    *(void **)(&real_open64) = dlsym(RTLD_NEXT, "__open64");

    return (*real_open64)(path, flags, mode);
}

}
