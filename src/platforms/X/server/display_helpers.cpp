/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "display_helpers.h"
#include "drm_close_threadsafe.h"

#include "mir/graphics/gl_config.h"
#include "mir/udev/wrapper.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <xf86drm.h>
#include <fcntl.h>

namespace mgx = mir::graphics::X;
namespace mgxh = mir::graphics::X::helpers;

/*************
 * DRMHelper *
 *************/

void mgxh::DRMHelper::setup(std::shared_ptr<mir::udev::Context> const& udev)
{
    fd = open_drm_device(udev);

    if (fd < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to open DRM render node device\n"));
}

mir::Fd mgxh::DRMHelper::authenticated_fd()
{
#if 0
    /* We must have our own device fd first, so that it has become the DRM master */
    if (fd < 0)
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Tried to get authenticated DRM fd before setting up the DRM master"));

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
#endif
    return mir::Fd{IntOwnedFd{-1}};
}

void mgxh::DRMHelper::auth_magic(drm_magic_t magic)
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
#if 0
void mgxh::DRMHelper::drop_master() const
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

void mgxh::DRMHelper::set_master() const
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

int mgxh::DRMHelper::is_appropriate_device(std::shared_ptr<mir::udev::Context> const& udev, mir::udev::Device const& drm_device)
{
    mir::udev::Enumerator children(udev);
    children.match_parent(drm_device);

    char const* devtype = drm_device.devtype();
    if (!devtype || strcmp(devtype, "drm_minor"))
        return EINVAL;

    children.scan_devices();
    for (auto& device : children)
    {
        // For some reason udev regards the device as a parent of itself
        // If there are any other children, they should be outputs.
        if (device != drm_device)
            return 0;
    }

    return ENOMEDIUM;
}

int mgxh::DRMHelper::count_connections(int fd)
{
    DRMModeResources resources{fd};

    int n_connected = 0;
    resources.for_each_connector([&](DRMModeConnectorUPtr connector)
    {
        if (connector->connection == DRM_MODE_CONNECTED)
            n_connected++;
    });

    return n_connected;
}
#endif
int mgxh::DRMHelper::open_drm_device(std::shared_ptr<mir::udev::Context> const& udev)
{
    int tmp_fd = -1;

    mir::udev::Enumerator devices(udev);
    devices.match_subsystem("drm");
    devices.match_sysname("renderD[0-9]*");

    devices.scan_devices();

    for(auto& device : devices)
    {
        // If directly opening the DRM device is good enough for X it's good enough for us!
        tmp_fd = open(device.devnode(), O_RDWR | O_CLOEXEC);
        if (tmp_fd < 0)
            continue;
        else
        	break;
    }

    return tmp_fd;
}

mgxh::DRMHelper::~DRMHelper()
{
    if (fd >= 0)
        mgx::drm_close_threadsafe(fd);
}

/*************
 * GBMHelper *
 *************/

void mgxh::GBMHelper::setup(const DRMHelper& drm)
{
    device = gbm_create_device(drm.fd);
    if (!device)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM device"));
}

void mgxh::GBMHelper::setup(int drm_fd)
{
    device = gbm_create_device(drm_fd);
    if(!device)
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Failed to create GBM device"));
}

mgx::GBMSurfaceUPtr mgxh::GBMHelper::create_scanout_surface(uint32_t width, uint32_t height)
{
    auto surface_raw = gbm_surface_create(device, width, height,
                                          GBM_BO_FORMAT_XRGB8888,
                                          GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

    auto gbm_surface_deleter = [](gbm_surface *p) { if (p) gbm_surface_destroy(p); };
    GBMSurfaceUPtr surface{surface_raw, gbm_surface_deleter};

    if (!surface)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GBM scanout surface"));

    return surface;
}

mgxh::GBMHelper::~GBMHelper()
{
    if (device)
        gbm_device_destroy(device);
}
