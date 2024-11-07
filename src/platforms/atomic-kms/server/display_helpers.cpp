/*
 * Copyright © Canonical Ltd.
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
 */

#include "display_helpers.h"
#include "one_shot_device_observer.h"

#include "kms-utils/drm_mode_resources.h"
#include "mir/graphics/egl_error.h"
#include "kms/quirks.h"

#include "mir/udev/wrapper.h"
#include "mir/console_services.h"

#include <sys/sysmacros.h>

#include "mir/log.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <stdexcept>
#include <xf86drm.h>
#include <fcntl.h>
#include <vector>
#include <boost/exception/diagnostic_information.hpp>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace mgc = mir::graphics::common;
namespace mgmh = mir::graphics::atomic::helpers;

/*************
 * DRMHelper *
 *************/

std::vector<std::shared_ptr<mgmh::DRMHelper>>
mgmh::DRMHelper::open_all_devices(
    std::shared_ptr<mir::udev::Context> const& udev,
    mir::ConsoleServices& console,
    mga::Quirks const& quirks)
{
    int error = ENODEV; //Default error is "there are no DRM devices"

    mir::udev::Enumerator devices(udev);
    devices.match_subsystem("drm");
    devices.match_sysname("card[0-9]");

    devices.scan_devices();

    std::vector<std::shared_ptr<DRMHelper>> opened_devices;

    for(auto& device : devices)
    {
        if (quirks.should_skip(device))
        {
            mir::log_info("Ignoring device %s due to specified quirk", device.devnode());
            continue;
        }

        mir::Fd tmp_fd;
        std::unique_ptr<mir::Device> device_handle;
        try
        {
            device_handle = console.acquire_device(
                major(device.devnum()), minor(device.devnum()),
                std::make_unique<mgc::OneShotDeviceObserver>(tmp_fd))
                    .get();
        }
        catch (std::exception const& error)
        {
            mir::log_warning(
                "Failed to open DRM device node %s: %s",
                device.devnode(),
                boost::diagnostic_information(error).c_str());
            continue;
        }

        // Paranoia is always justified when dealing with hardware interfaces…
        if (tmp_fd == mir::Fd::invalid)
        {
            mir::log_critical(
                "Opening the DRM device %s succeeded, but provided an invalid descriptor!",
                device.devnode());
            mir::log_critical(
                "This is probably a logic error in Mir, please report to https://github.com/MirServer/mir/issues");
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

        if (!busid)
        {
            mir::log_warning(
                "Failed to query BusID for device %s; cannot check if KMS is available",
                device.devnode());
        }
        else
        {
            switch (auto err = -drmCheckModesettingSupported(busid.get()))
            {
            case 0: break;

            case ENOSYS:
                if (quirks.require_modesetting_support(device))
                {
                    mir::log_info("Ignoring non-KMS DRM device %s", device.devnode());
                    error = ENOSYS;
                    continue;
                }

                [[fallthrough]];
            case EINVAL:
                mir::log_warning(
                    "Failed to detect whether device %s supports KMS, but continuing anyway",
                    device.devnode());
                break;

            default:
                mir::log_warning("Unexpected error from drmCheckModesettingSupported(): %s (%i), but continuing anyway",
                    strerror(err), err);
                mir::log_warning("Please file a bug at https://github.com/MirServer/mir/issues containing this message");
            }
        }

        // Can't use make_shared with the private constructor.
        opened_devices.push_back(
            std::shared_ptr<DRMHelper>{
                new DRMHelper{
                    std::move(tmp_fd),
                    std::move(device_handle)}});
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
        new mgmh::DRMHelper{std::move(tmp_fd), nullptr}};
}

mgmh::DRMHelper::DRMHelper(mir::Fd&& fd, std::unique_ptr<mir::Device> device)
    : fd{std::move(fd)},
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

mgmh::GBMHelper::~GBMHelper()
{
    if (device)
        gbm_device_destroy(device);
}
