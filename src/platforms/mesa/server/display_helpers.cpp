/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_helpers.h"
#include "one_shot_device_observer.h"

#include "kms-utils/drm_mode_resources.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"

#include "mir/udev/wrapper.h"
#include "mir/console_services.h"

#include <sys/sysmacros.h>

#define MIR_LOG_COMPONENT "mesa-kms"
#include "mir/log.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <xf86drm.h>
#include <fcntl.h>
#include <vector>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mgc = mir::graphics::common;
namespace mgmh = mir::graphics::mesa::helpers;

/*************
 * DRMHelper *
 *************/

std::vector<std::shared_ptr<mgmh::DRMHelper>>
mgmh::DRMHelper::open_all_devices(
    std::shared_ptr<mir::udev::Context> const& udev,
    mir::ConsoleServices& console)
{
    int error = ENODEV; //Default error is "there are no DRM devices"

    mir::udev::Enumerator devices(udev);
    devices.match_subsystem("drm");
    devices.match_sysname("card[0-9]");

    devices.scan_devices();

    std::vector<std::shared_ptr<DRMHelper>> opened_devices;

    for(auto& device : devices)
    {
        mir::Fd tmp_fd;
        auto device_handle = console.acquire_device(
            major(device.devnum()), minor(device.devnum()),
            std::make_unique<mgc::OneShotDeviceObserver>(tmp_fd)).get();

        if (tmp_fd < 0)
        {
            error = errno;
            mir::log_warning(
                "Failed to open DRM device node %s: %i (%s)",
                device.devnode(),
                error,
                strerror(error));
            continue;
        }

        // Check that the drm device is usable by setting the interface version we use (1.4)
        drmSetVersion sv;
        sv.drm_di_major = 1;
        sv.drm_di_minor = 4;
        sv.drm_dd_major = -1;     /* Don't care */
        sv.drm_dd_minor = -1;     /* Don't care */

        if ((error = -drmSetInterfaceVersion(tmp_fd, &sv)))
        {
            mir::log_warning(
                "Failed to set DRM interface version on device %s: %i (%s)",
                device.devnode(),
                error,
                strerror(error));
            continue;
        }

        auto busid = std::unique_ptr<char, decltype(&drmFreeBusid)>{
            drmGetBusid(tmp_fd), &drmFreeBusid
        };

        if (getenv("MIR_MESA_KMS_DISABLE_MODESET_PROBE") == nullptr)
        {
            if (auto modeset_error = -drmCheckModesettingSupported(busid.get()))
            {
                mir::log_info("Ignoring non-KMS DRM device %s", device.devnode());
                error = modeset_error;
                continue;
            }
        }

        // Can't use make_shared with the private constructor.
        opened_devices.push_back(
            std::shared_ptr<DRMHelper>{
                new DRMHelper{
                    std::move(tmp_fd),
                    std::move(device_handle),
                    DRMNodeToUse::card}});
        mir::log_info("Using DRM device %s", device.devnode());
    }

    if (opened_devices.size() == 0)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{error, std::system_category(), "Error opening DRM device"}));
    }

    return opened_devices;
}

std::unique_ptr<mgmh::DRMHelper> mgmh::DRMHelper::open_any_render_node(
    std::shared_ptr<mir::udev::Context> const& udev)
{
    mir::Fd tmp_fd;
    int error = ENODEV; //Default error is "there are no DRM devices"

    mir::udev::Enumerator devices(udev);
    devices.match_subsystem("drm");
    devices.match_sysname("renderD[0-9]*");

    devices.scan_devices();

    for(auto& device : devices)
    {
        // If directly opening the DRM device is good enough for X it's good enough for us!
        tmp_fd = mir::Fd{open(device.devnode(), O_RDWR | O_CLOEXEC)};
        if (tmp_fd < 0)
        {
            error = errno;
            continue;
        }

        break;
    }

    if (tmp_fd < 0)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                    error,
                    std::system_category(),
                    "Error opening DRM device"}));
    }

    return std::unique_ptr<mgmh::DRMHelper>{
        new mgmh::DRMHelper{std::move(tmp_fd), nullptr, DRMNodeToUse::render}};
}

mir::Fd mgmh::DRMHelper::authenticated_fd()
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Tried to get authenticated DRM fd before setting up the DRM master"));

    if (node_to_use == DRMNodeToUse::render)
        return fd;

    char* busid = drmGetBusid(fd);
    if (!busid)
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get BusID of DRM device")) << boost::errinfo_errno(errno));
    int auth_fd = drmOpen(NULL, busid);
    drmFreeBusid(busid);

    if (auth_fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to open DRM device for authenticated fd"));

    if (fcntl(auth_fd, F_SETFD, fcntl(auth_fd, F_GETFD) | FD_CLOEXEC) == -1)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set FD_CLOEXEC for authenticated drm fd")));
    }

    drm_magic_t magic;
    int ret = -1;
    if ((ret = drmGetMagic(auth_fd, &magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }

    if ((ret = drmAuthMagic(fd, magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to authenticate DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }

    return mir::Fd{auth_fd};
}

void mgmh::DRMHelper::auth_magic(drm_magic_t magic)
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Tried to authenticate magic cookie before setting up the DRM master"));
    }

    int ret = drmAuthMagic(fd, magic);

    if (ret < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to authenticate DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }
}

void mgmh::DRMHelper::drop_master() const
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Tried to drop DRM master without a DRM device"));
    }

    int ret = drmDropMaster(fd);

    if (ret < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to drop DRM master"))
                    << boost::errinfo_errno(errno));
    }
}

void mgmh::DRMHelper::set_master() const
{
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Tried to set DRM master without a DRM device"));
    }

    int ret = drmSetMaster(fd);

    if (ret < 0)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to set DRM master"))
                    << boost::errinfo_errno(errno));
    }
}

mgmh::DRMHelper::DRMHelper(mir::Fd&& fd, std::unique_ptr<mir::Device> device, DRMNodeToUse node_to_use)
    : fd{std::move(fd)},
      node_to_use{node_to_use},
      device_handle{std::move(device)}
{
}

mgmh::DRMHelper::~DRMHelper()
{
}

/*************
 * GBMHelper *
 *************/

mgmh::GBMHelper::GBMHelper(mir::Fd const& drm_fd)
    : device{gbm_create_device(drm_fd)}
{
    if (!device)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM device"));
}

mgm::GBMSurfaceUPtr mgmh::GBMHelper::create_scanout_surface(
    uint32_t width,
    uint32_t height,
    bool sharable)
{
    auto format_flags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;

    if (sharable)
    {
#ifdef MIR_NO_HYBRID_SUPPORT
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                "Mir built without hybrid support, but configuration requires hybrid outputs.\n"
                "This will not work unless Mir is rebuilt against Mesa >= 11.0"}
            ));
#else
        format_flags |= GBM_BO_USE_LINEAR;
#endif
    }

    auto surface_raw = gbm_surface_create(device, width, height,
                                          GBM_BO_FORMAT_XRGB8888,
                                          format_flags);

    auto gbm_surface_deleter = [](gbm_surface *p) { if (p) gbm_surface_destroy(p); };
    GBMSurfaceUPtr surface{surface_raw, gbm_surface_deleter};

    if (!surface)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GBM scanout surface"));

    return surface;
}

mgmh::GBMHelper::~GBMHelper()
{
    if (device)
        gbm_device_destroy(device);
}
