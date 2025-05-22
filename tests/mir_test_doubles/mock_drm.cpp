/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/test/doubles/mock_drm.h"
#include "mir_test_framework/mmap_wrapper.h"
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
    uint32_t const fb_id{40};
    uint32_t const plane_id{50};
    uint32_t const gamma_size{0};

    auto type_prop_id = add_property("type");

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

    plane_formats.push_back(0);
    add_plane(plane_formats, plane_id, crtc0_id, fb_id, 0, 0, 0, 0, all_crtcs_mask, gamma_size, { type_prop_id }, { DRM_PLANE_TYPE_PRIMARY });

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

drmModePlaneRes* mtd::FakeDRMResources::plane_resources_ptr()
{
    return &plane_resources;
}

drmModeObjectProperties* mtd::FakeDRMResources::get_object_properties(uint32_t id, uint32_t type)
{
    auto props =
        std::unique_ptr<drmModeObjectProperties, void (*)(drmModeObjectProperties*)>(new drmModeObjectProperties, &::drmModeFreeObjectProperties);

    auto const& object = objects.at(id);
    if (object.object_type != type)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Attempt to look up DRM object with incorrect type"});
    }

    props->count_props = object.ids.size();
    props->props = const_cast<uint32_t*>(object.ids.data());
    props->prop_values = const_cast<uint64_t*>(object.values.data());

    allocated_obj_props.insert(props.get());
    return props.release();
}

drmModePropertyRes* mtd::FakeDRMResources::get_property(uint32_t prop_id)
{
    auto const& property = properties.at(prop_id);

    auto prop =
       std::unique_ptr<drmModePropertyRes, void (*)(drmModePropertyPtr)>(new drmModePropertyRes, &::drmModeFreeProperty);

    memset(prop.get(), 0, sizeof(*prop));

    prop->prop_id = prop_id;
    strncpy(prop->name, property.c_str(), sizeof(prop->name) - 1);

    allocated_props.insert(prop.get());
    return prop.release();
}

drmModePropertyBlobRes* mtd::FakeDRMResources::get_property_blob(uint32_t prop_id)
{
    auto blob =
        std::unique_ptr<drmModePropertyBlobRes, void (*)(drmModePropertyBlobPtr)>(new drmModePropertyBlobRes, &::drmModeFreePropertyBlob);

    memset(blob.get(), 0, sizeof(*blob));

    blob->id = prop_id;
    blob->length = 0;
    blob->data = NULL;

    allocated_prop_blobs.insert(blob.get());
    return blob.release();
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

    plane_resources.count_planes = planes.size();
    for (auto const& plane: planes)
        plane_ids.push_back(plane.plane_id);
    plane_resources.planes = plane_ids.data();
}

void mtd::FakeDRMResources::reset()
{
    resources = drmModeRes();
    plane_resources = drmModePlaneRes();

    crtcs.clear();
    encoders.clear();
    connectors.clear();
    planes.clear();
    objects.clear();
    properties.clear();
    allocated_obj_props.clear();
    allocated_props.clear();
    allocated_prop_blobs.clear();

    crtc_ids.clear();
    encoder_ids.clear();
    connector_ids.clear();
    plane_ids.clear();
}

uint32_t mtd::FakeDRMResources::add_property(char const* name)
{
    properties.insert({next_prop_id, name});
    return next_prop_id++;
}

void mtd::FakeDRMResources::add_crtc(uint32_t id, drmModeModeInfo mode,
                                     std::vector<uint32_t> prop_ids,
                                     std::vector<uint64_t> prop_values)
{
    drmModeCrtc crtc = drmModeCrtc();

    crtc.crtc_id = id;
    crtc.mode = mode;

    crtcs.push_back(crtc);

    objects[id] = {DRM_MODE_OBJECT_CRTC, prop_ids, prop_values};
}

void mtd::FakeDRMResources::add_encoder(uint32_t encoder_id, uint32_t crtc_id,
                                        uint32_t possible_crtcs_mask,
                                        std::vector<uint32_t> prop_ids,
                                        std::vector<uint64_t> prop_values)
{
    drmModeEncoder encoder = drmModeEncoder();

    encoder.encoder_id = encoder_id;
    encoder.crtc_id = crtc_id;
    encoder.possible_crtcs = possible_crtcs_mask;

    encoders.push_back(encoder);

    objects[encoder_id] = {DRM_MODE_OBJECT_ENCODER, prop_ids, prop_values};
}

void mtd::FakeDRMResources::add_connector(uint32_t connector_id,
                                          uint32_t type,
                                          drmModeConnection connection,
                                          uint32_t encoder_id,
                                          std::vector<drmModeModeInfo>& modes,
                                          std::vector<uint32_t>& possible_encoder_ids,
                                          geom::Size const& physical_size,
                                          drmModeSubPixel subpixel_arrangement,
                                          std::vector<uint32_t> prop_ids,
                                          std::vector<uint64_t> prop_values)
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

    objects[connector_id] = {DRM_MODE_OBJECT_CONNECTOR, prop_ids, prop_values};
}

void mtd::FakeDRMResources::add_plane(std::vector<uint32_t>& formats,
                                      uint32_t plane_id,
                                      uint32_t crtc_id,
                                      uint32_t fb_id,
                                      uint32_t crtc_x,
                                      uint32_t crtc_y,
                                      uint32_t x,
                                      uint32_t y,
                                      uint32_t possible_crtcs_mask,
                                      uint32_t gamma_size,
                                      std::vector<uint32_t> prop_ids,
                                      std::vector<uint64_t> prop_values)
{
    drmModePlane plane = drmModePlane();

    plane.count_formats = formats.size();
    plane.formats = formats.data();
    plane.plane_id = plane_id;
    plane.crtc_id = crtc_id;
    plane.fb_id = fb_id;
    plane.crtc_x = crtc_x;
    plane.crtc_y = crtc_y;
    plane.x = x;
    plane.y = y;
    plane.possible_crtcs = possible_crtcs_mask;
    plane.gamma_size = gamma_size;

    planes.push_back(plane);

    objects[plane_id] = {DRM_MODE_OBJECT_PLANE, prop_ids, prop_values};
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

drmModePlane* mtd::FakeDRMResources::find_plane(uint32_t id)
{
    for (auto& plane : planes)
    {
        if (plane.plane_id == id)
            return &plane;
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
        [this](char const* path, int flags, std::optional<mode_t>) -> std::optional<int>
        {
            char const* const drm_prefix = "/dev/dri/";
            if (!strncmp(path, drm_prefix, strlen(drm_prefix)))
            {
                // I don't think we need to be able to distinguish based on mode. ppc64el (at least) *does*
                // call the 3-parameter open()
                return this->open(path, flags);
            }
            return std::nullopt;
        })},
      mmap_interposer{mir_test_framework::add_mmap_handler(
        [this](void* addr, size_t length, int prot, int flags, int fd, off_t offset) -> std::optional<void*>
        {
            // The dumb buffer API uses a call of mmap() on the DRM fd, so we want to handle that.
            if (this->fd_to_drm.contains(fd))
            {
                return this->mmap(addr, length, prot, flags, fd, offset);
            }
            return std::nullopt;
        })},
      munmap_interposer{mir_test_framework::add_munmap_handler(
        [this](void* addr, size_t length) -> std::optional<int>
        {
            auto maybe_mapping = mmapings.find(addr);
            if (maybe_mapping != mmapings.end())
            {
                if (maybe_mapping->second != length)
                {
                    /* It's technically possible to unmap only *part* of the mapping
                     * A more fancy mock would check whether addr + length overlaps
                     * any existing mappings and punch holes, but 🤷‍♀️
                     */
                    BOOST_THROW_EXCEPTION((std::runtime_error{"munmap mock does not support partial unmappings"}));
                }
                return this->munmap(addr, length);
            }
            return std::nullopt;
        })}
{
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, open(_,_))
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

    ON_CALL(*this, drmModeGetPlaneResources(_))
        .WillByDefault(
            Invoke(
                [this](int fd)
                {
                    return fd_to_drm.at(fd).plane_resources_ptr();
                }));

    ON_CALL(*this, drmModeGetPlane(_, _))
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t plane_id)
                {
                    return fd_to_drm.at(fd).find_plane(plane_id);
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
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t id, uint32_t type)
                {
                    return fd_to_drm.at(fd).get_object_properties(id, type);
                }));

    ON_CALL(*this, drmModeFreeObjectProperties(_))
        .WillByDefault(
            Invoke(
                [this](drmModeObjectPropertiesPtr props)
                {
                    delete props;
                }));

    ON_CALL(*this, drmModeGetProperty(_, _))
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t prop_id)
                {
                    return fd_to_drm.at(fd).get_property(prop_id);
                }));

    ON_CALL(*this, drmModeFreeProperty(_))
        .WillByDefault(
            Invoke(
                [this](drmModePropertyPtr prop)
                {
                    delete prop;
                }));

    ON_CALL(*this, drmModeGetPropertyBlob(_, _))
        .WillByDefault(
            Invoke(
                [this](int fd, uint32_t prop_id)
                {
                    return fd_to_drm.at(fd).get_property_blob(prop_id);
                }));

    ON_CALL(*this, drmModeFreePropertyBlob(_))
        .WillByDefault(
            Invoke(
                [this](drmModePropertyBlobPtr prop)
                {
                    delete prop;
                }));

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

    ON_CALL(*this, mmap(_, _, _, _, _, _))
        .WillByDefault(
            WithArg<1>(Invoke(
                [this](auto size) -> void*
                {
                    auto allocation = std::make_unique<char[]>(size);
                    void* const address = allocation.get();
                    mmapings.insert(std::make_pair(std::move(allocation), size));
                    return address;
                })));

    ON_CALL(*this, munmap(_, _))
        .WillByDefault(
            WithArg<0>(Invoke(
                [this](void* addr) -> int
                {
                    auto iter = mmapings.find(addr);
                    mmapings.erase(iter);
                    return 0;
                })));

    ON_CALL(*this, drmGetCap(_, DRM_CAP_DUMB_BUFFER, _))
        .WillByDefault(
            [](auto, auto, uint64_t* value)
            {
                *value = 1;
                return 0;
            });
}

mtd::MockDRM::~MockDRM() noexcept
{
    global_mock = nullptr;
}

uint32_t mtd::MockDRM::add_property(char const *device, char const* name)
{
    return fake_drms[device].add_property(name);
}

void mtd::MockDRM::add_crtc(
    char const *device,
    uint32_t id,
    drmModeModeInfo mode,
    std::vector<uint32_t> prop_ids,
    std::vector<uint64_t> prop_values)
{
    fake_drms[device].add_crtc(id, mode, prop_ids, prop_values);
}

void mtd::MockDRM::add_encoder(
    char const *device,
    uint32_t encoder_id,
    uint32_t crtc_id,
    uint32_t possible_crtcs_mask,
    std::vector<uint32_t> prop_ids,
    std::vector<uint64_t> prop_values)
{
    fake_drms[device].add_encoder(encoder_id, crtc_id, possible_crtcs_mask, prop_ids, prop_values);
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
    drmModeSubPixel subpixel_arrangement,
    std::vector<uint32_t> prop_ids,
    std::vector<uint64_t> prop_values)
{
    fake_drms[device].add_connector(
        connector_id,
        type,
        connection,
        encoder_id,
        modes,
        possible_encoder_ids,
        physical_size,
        subpixel_arrangement,
        prop_ids,
        prop_values);
}

void mtd::MockDRM::add_plane(
    char const *device,
    std::vector<uint32_t>& formats,
    uint32_t plane_id,
    uint32_t crtc_id,
    uint32_t fb_id,
    uint32_t crtc_x,
    uint32_t crtc_y,
    uint32_t x,
    uint32_t y,
    uint32_t possible_crtcs_mask,
    uint32_t gamma_size,
    std::vector<uint32_t> prop_ids,
    std::vector<uint64_t> prop_values)
{
    fake_drms[device].add_plane(
        formats,
        plane_id,
        crtc_id,
        fb_id,
        crtc_x,
        crtc_y,
        x,
        y,
        possible_crtcs_mask,
        gamma_size,
        prop_ids,
        prop_values);
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

// The signature of drmModeCrtcSetGamma() changes from passing the gamma as `uint16_t*` to `uint16_t const*`
// We need to provide a definition that matches
namespace
{
template<typename Any>
struct Decode_drmModeCrtcSetGamma_signature;

template<typename Arg>
struct Decode_drmModeCrtcSetGamma_signature<int(int fd, uint32_t crtc_id, uint32_t size, Arg red, Arg gree, Arg blue)>
{
    using gamma_t = Arg;
};
using gamma_t = Decode_drmModeCrtcSetGamma_signature<decltype(drmModeCrtcSetGamma)>::gamma_t;
}

// Ensure we get a compile error if the definition doesn't match the declaration (instead of providing an overload)
extern "C"
{
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

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size, gamma_t red, gamma_t green, gamma_t blue)
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

void drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr)
{
    global_mock->drmModeFreePropertyBlob(ptr);
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

drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blobId)
{
    return global_mock->drmModeGetPropertyBlob(fd, blobId);
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

int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, uint32_t const bo_handles[4],
                  uint32_t const pitches[4], uint32_t const offsets[4],
                  uint32_t* buf_id, uint32_t flags)
{
    return global_mock->drmModeAddFB2(fd, width, height, pixel_format,
                                      bo_handles, pitches, offsets,
                                      buf_id, flags);
}

int drmModeAddFB2WithModifiers(
    int fd,
    uint32_t width,
    uint32_t height,
    uint32_t fourcc,
    uint32_t const handles[4],
    uint32_t const pitches[4],
    uint32_t const offsets[4],
    uint64_t const modifiers[4],
    uint32_t *buf_id,
    uint32_t flags)
{
    return global_mock->drmModeAddFB2WithModifiers(
        fd, width, height, fourcc, handles, pitches, offsets, modifiers, buf_id, flags);
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
}
