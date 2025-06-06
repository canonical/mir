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

#include "platform.h"
#include "display.h"
#include "mir/console_services.h"
#include "mir/emergency_cleanup_registry.h"
#include "mir/graphics/platform.h"
#include "mir/udev/wrapper.h"
#include "one_shot_device_observer.h"
#include <boost/throw_exception.hpp>
#include <system_error>
#include <xf86drm.h>

#include "mir/log.h"

namespace mg = mir::graphics;
namespace mga = mg::atomic;

namespace
{
auto master_fd_for_device(mir::udev::Device const& device, mir::ConsoleServices& vt) -> std::tuple<std::unique_ptr<mir::Device>, mir::Fd>
{
    mir::Fd drm_fd;
    auto device_handle = vt.acquire_device(
        major(device.devnum()), minor(device.devnum()),
        std::make_unique<mg::common::OneShotDeviceObserver>(drm_fd))
            .get();

    if (drm_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to acquire DRM fd"}));
    }

    if (auto err = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to enable DRM Universal Planes support"}));
    }

    if (auto err = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to enable Atomic KMS support"}));
    }

    return std::make_tuple(std::move(device_handle), std::move(drm_fd));
}

auto maybe_make_gbm_provider(mir::Fd drm_fd) -> std::shared_ptr<mga::GBMDisplayProvider>
{
    try
    {
        return std::make_shared<mga::GBMDisplayProvider>(std::move(drm_fd));
    }
    catch (std::exception const& err)
    {
        mir::log_info("Failed to create GBM device for direct buffer submission");
        mir::log_info("Output will use CPU buffer copies");
        return {};
    }
}
}

mga::Platform::Platform(
    udev::Device const& device,
    std::shared_ptr<DisplayReport> const& listener,
    ConsoleServices& vt,
    EmergencyCleanupRegistry& registry,
    BypassOption bypass_option,
    std::shared_ptr<GbmQuirks> runtime_quirks)
    : Platform(master_fd_for_device(device, vt), listener, registry, bypass_option, runtime_quirks)
{
}

mga::Platform::Platform(
    std::tuple<std::unique_ptr<Device>, mir::Fd> drm,
    std::shared_ptr<DisplayReport> const& listener,
    EmergencyCleanupRegistry&,
    BypassOption bypass_option,
    std::shared_ptr<GbmQuirks> runtime_quirks)
    : udev{std::make_shared<mir::udev::Context>()},
      listener{listener},
      device_handle{std::move(std::get<0>(drm))},
      drm_fd{std::move(std::get<1>(drm))},
      gbm_display_provider{maybe_make_gbm_provider(drm_fd)},
      bypass_option_{bypass_option},
      runtime_quirks{runtime_quirks}
{
    if (drm_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Invalid DRM device FD"}));
    }
}

mga::Platform::~Platform() = default;

namespace
{
auto gbm_device_from_provider(std::shared_ptr<mg::GBMDisplayProvider> const& provider)
    -> std::shared_ptr<struct gbm_device>
{
    if (provider)
    {
        return provider->gbm_device();
    }
    return nullptr;
}
}

mir::UniqueModulePtr<mg::Display> mga::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy, std::shared_ptr<GLConfig> const&)
{
    return make_module_ptr<mga::Display>(
        drm_fd,
        gbm_device_from_provider(gbm_display_provider),
        bypass_option_,
        initial_conf_policy,
        listener,
        runtime_quirks);
}

auto mga::Platform::maybe_create_provider(DisplayProvider::Tag const& type_tag)
    -> std::shared_ptr<DisplayProvider>
{
    if (dynamic_cast<mg::GBMDisplayProvider::Tag const*>(&type_tag))
    {
        return gbm_display_provider;
    }
    if (dynamic_cast<mg::CPUAddressableDisplayProvider::Tag const*>(&type_tag))
    {
        /* There's no implementation behind it, but we want to know during probe time
         * that the DisplayBuffers will support it.
         */
        return std::make_shared<CPUAddressableDisplayProvider>();
    }
    return {};
}

mga::BypassOption mga::Platform::bypass_option() const
{
    return bypass_option_;
}
