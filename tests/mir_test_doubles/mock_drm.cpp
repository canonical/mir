/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/doubles/mock_drm.h"
#include "mir_test_framework/open_wrapper.h"
#include "mir/geometry/size.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include <unistd.h>
#include <dlfcn.h>
#include <system_error>
#include <boost/throw_exception.hpp>

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
                                          geom::Size const& physical_size,
                                          drmModeSubPixel subpixel_arrangement)
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
    connector.subpixel = subpixel_arrangement;

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
    : open_interposer{mir_test_framework::add_open_handler(
        [this](char const* path, int flags, mode_t mode) -> std::experimental::optional<int>
        {
            char const* const drm_prefix = "/dev/dri/";
            if (!strncmp(path, drm_prefix, strlen(drm_prefix)))
            {
                return this->open(path, flags, mode);
            }
            return {};
        })}
{
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;

    memset(&empty_object_props, 0, sizeof(empty_object_props));

    ON_CALL(*this, open(_,_,_))
        .WillByDefault(
            WithArg<0>(
                Invoke(
                    [this](char const* device_path)
                    {
                        auto fd = fake_drms[device_path].fd();
                        fd_to_drm.insert({fd, fake_drms[device_path]});
                        return fd;
                    })));

    ON_CALL(*this, drmOpen(_,_))
        .WillByDefault(
            InvokeWithoutArgs(
                [this]()
                {
                    auto fd = fake_drms["/dev/dri/card0"].fd();
                    fd_to_drm.insert({fd, fake_drms["/dev/dri/card0"]});
                    return fd;
                }));

    ON_CALL(*this, drmModeGetResources(_))
        .WillByDefault(
            Invoke(
                [this](int fd)
                {
                    return fd_to_drm.at(fd).resources_ptr();
                }));

    ON_CALL(*this, drmModeGetCrtc(_, _))
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t crtc_id)
                {
                    return fd_to_drm.at(fd).find_crtc(crtc_id);
                }));

    ON_CALL(*this, drmModeGetEncoder(_, _))
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t encoder_id)
                {
                    return fd_to_drm.at(fd).find_encoder(encoder_id);
                }));

    ON_CALL(*this, drmModeGetConnector(_, _))
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t connector_id)
                {
                    return fd_to_drm.at(fd).find_connector(connector_id);
                }));

    ON_CALL(*this, drmModeObjectGetProperties(_, _, _))
        .WillByDefault(Return(&empty_object_props));

    ON_CALL(*this, drmSetInterfaceVersion(_, _))
        .WillByDefault(
            Invoke(
                [this](auto fd, auto version)
                {
                    if (version->drm_di_major != 1 || version->drm_di_minor != 4)
                    {
                        ADD_FAILURE() << "We should only ever request DRM version 1.4";
                        errno = EINVAL;
                        return -1;
                    }

                    fd_to_drm.at(fd).drm_setversion_called = true;
                    return 0;
                }));

    ON_CALL(*this, drmGetBusid(_))
        .WillByDefault(
            Invoke(
                [this](auto fd) -> char*
                {
                    if (!fd_to_drm.at(fd).drm_setversion_called)
                    {
                        return nullptr;
                    }
                    return static_cast<char*>(malloc(10));
                }));

    ON_CALL(*this, drmFreeBusid(_))
        .WillByDefault(WithArg<0>(Invoke([&](const char* busid) { free(const_cast<char*>(busid)); })));

    ON_CALL(*this, drmGetPrimaryDeviceNameFromFd(_))
        .WillByDefault(InvokeWithoutArgs([]() { return strdup("/dev/dri/card0"); }));

    static drmVersion const version{
        1,
        2,
        3,
        static_cast<int>(strlen("mock_driver")),
        const_cast<char*>("mock_driver"),
        static_cast<int>(strlen("1 Jan 1970")),
        const_cast<char*>("1 Jan 1970"),
        static_cast<int>(strlen("Not really a driver")),
        const_cast<char*>("Not really a driver")
    };
    ON_CALL(*this, drmGetVersion(_))
        .WillByDefault(Return(const_cast<drmVersionPtr>(&version)));

    ON_CALL(*this, drmCheckModesettingSupported(NotNull()))
        .WillByDefault(Return(0));
    ON_CALL(*this, drmCheckModesettingSupported(IsNull()))
        .WillByDefault(Return(-EINVAL));
}

mtd::MockDRM::~MockDRM() noexcept
{
    global_mock = nullptr;
}

void mtd::MockDRM::add_crtc(char const *device, uint32_t id, drmModeModeInfo mode)
{
    fake_drms[device].add_crtc(id, mode);
}

void mtd::MockDRM::add_encoder(
    char const *device,
    uint32_t encoder_id,
    uint32_t crtc_id,
    uint32_t possible_crtcs_mask)
{
    fake_drms[device].add_encoder(encoder_id, crtc_id, possible_crtcs_mask);
}

void mtd::MockDRM::prepare(char const *device)
{
    fake_drms[device].prepare();
}

void mtd::MockDRM::reset(char const *device)
{
    fake_drms[device].reset();
}

void mtd::MockDRM::generate_event_on(char const *device)
{
    auto const fd = fake_drms[device].write_fd();

    if (write(fd, "a", 1) != 1)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to make fake DRM event"));
    }
}

void mtd::MockDRM::consume_event_on(char const* device)
{
    auto const fd = fake_drms[device].fd();

    char consume_buf;
    if (::read(fd, &consume_buf, 1) != 1)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to consume fake DRM event"));
    }
}

void mtd::MockDRM::add_connector(
    char const *device,
    uint32_t connector_id,
    uint32_t type,
    drmModeConnection connection,
    uint32_t encoder_id,
    std::vector<drmModeModeInfo> &modes,
    std::vector<uint32_t> &possible_encoder_ids,
    geometry::Size const &physical_size,
    drmModeSubPixel subpixel_arrangement)
{
    fake_drms[device].add_connector(
        connector_id,
        type,
        connection,
        encoder_id,
        modes,
        possible_encoder_ids,
        physical_size,
        subpixel_arrangement);
}

MATCHER_P2(IsFdOfDevice, devname, fds, "")
{
    return std::find(
        fds.begin(),
        fds.end(),
        arg) != fds.end();
}

class mtd::MockDRM::IsFdOfDeviceMatcher : public ::testing::MatcherInterface<int>
{
public:
    IsFdOfDeviceMatcher(char const* device)
        : device{device}
    {
    }

    void DescribeTo(::std::ostream *os) const override
    {

        *os
            << "Is an fd of DRM device "
            << device
            << " (one of: "
            << ::testing::PrintToString(fds_for_device(device.c_str()))
            << ")";
    }

    bool MatchAndExplain(int x, testing::MatchResultListener *listener) const override
    {
        testing::Matcher<std::vector<int>> matcher = testing::Contains(x);
        return matcher.MatchAndExplain(fds_for_device(device.c_str()), listener);
    }

private:
    std::vector<int> fds_for_device(char const* device) const
    {
        std::vector<int> device_fds;
        for (auto const& pair : global_mock->fd_to_drm)
        {
            if (&global_mock->fake_drms[device] == &pair.second)
            {
                device_fds.push_back(pair.first);
            }
        }
        return device_fds;
    }

    std::string const device;
};

testing::Matcher<int> mtd::IsFdOfDevice(char const* device)
{
    return ::testing::MakeMatcher(new mtd::MockDRM::IsFdOfDeviceMatcher(device));
}

int drmOpen(const char *name, const char *busid)
{
    return global_mock->drmOpen(name, busid);
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

drmModePlaneResPtr drmModeGetPlaneResources(int fd)
{
    return global_mock->drmModeGetPlaneResources(fd);
}

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id)
{
    return global_mock->drmModeGetPlane(fd, plane_id);
}

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd, uint32_t id, uint32_t type)
{
    return global_mock->drmModeObjectGetProperties(fd, id, type);
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

int drmModeCrtcGetGamma(int fd, uint32_t crtc_id, uint32_t size,
                        uint16_t* red, uint16_t* green, uint16_t* blue)
{
    return global_mock->drmModeCrtcGetGamma(fd, crtc_id, size, red, green, blue);
}

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size,
                        uint16_t* red, uint16_t* green, uint16_t* blue)
{
    return global_mock->drmModeCrtcSetGamma(fd, crtc_id, size, red, green, blue);
}

void drmModeFreeResources(drmModeResPtr ptr)
{
    global_mock->drmModeFreeResources(ptr);
}

void drmModeFreeProperty(drmModePropertyPtr ptr)
{
    global_mock->drmModeFreeProperty(ptr);
}

int drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
    return global_mock->drmGetCap(fd, capability, value);
}

drmVersionPtr drmGetVersion(int fd)
{
    return global_mock->drmGetVersion(fd);
}

void drmFreeVersion(drmVersionPtr version)
{
    return global_mock->drmFreeVersion(version);
}

int drmSetClientCap(int fd, uint64_t capability, uint64_t value)
{
    return global_mock->drmSetClientCap(fd, capability, value);
}

drmModePropertyPtr drmModeGetProperty(int fd, uint32_t propertyId)
{
    return global_mock->drmModeGetProperty(fd, propertyId);
}

int drmModeConnectorSetProperty(int fd, uint32_t connector_id, uint32_t property_id, uint64_t value)
{
    return global_mock->drmModeConnectorSetProperty(fd, connector_id, property_id, value);
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

void drmModeFreePlaneResources(drmModePlaneResPtr ptr)
{
    global_mock->drmModeFreePlaneResources(ptr);
}

void drmModeFreePlane(drmModePlanePtr ptr)
{
    global_mock->drmModeFreePlane(ptr);
}

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr)
{
    global_mock->drmModeFreeObjectProperties(ptr);
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height,
                 uint8_t depth, uint8_t bpp, uint32_t pitch,
                 uint32_t bo_handle, uint32_t *buf_id)
{

    return global_mock->drmModeAddFB(fd, width, height, depth, bpp,
                                     pitch, bo_handle, buf_id);
}

#ifdef MIR_DRMMODEADDFB_HAS_CONST_SIGNATURE
int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, uint32_t const bo_handles[4],
                  uint32_t const pitches[4], uint32_t const offsets[4],
                  uint32_t* buf_id, uint32_t flags)
#else
int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, uint32_t bo_handles[4],
                  uint32_t pitches[4], uint32_t offsets[4],
                  uint32_t *buf_id, uint32_t flags)
#endif
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

int drmIsMaster(int fd)
{
    return global_mock->drmIsMaster(fd);
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

void drmFreeBusid(const char* busid)
{
    return global_mock->drmFreeBusid(busid);
}

char* drmGetDeviceNameFromFd(int fd)
{
    return global_mock->drmGetDeviceNameFromFd(fd);
}

char* drmGetPrimaryDeviceNameFromFd(int fd)
{
    return global_mock->drmGetPrimaryDeviceNameFromFd(fd);
}

int drmCheckModesettingSupported(char const* busid)
{
    return global_mock->drmCheckModesettingSupported(busid);
}
