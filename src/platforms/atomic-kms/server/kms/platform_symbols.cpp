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

#include "mir/graphics/platform.h"
#include <drm.h>
#include "mir/log.h"

#include "platform.h"
#include "display_helpers.h"
#include "quirks.h"
#include "kms-utils/drm_mode_resources.h"
#include "mir/options/program_option.h"
#include "mir/options/option.h"
#include "mir/options/configuration.h"
#include "mir/udev/wrapper.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/console_services.h"
#include "one_shot_device_observer.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_logger.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "egl_helper.h"
#include <fcntl.h>
#include <xf86drm.h>

namespace mg = mir::graphics;
namespace mga = mg::atomic;
namespace mgc = mir::graphics::common;
namespace mo = mir::options;
namespace mgk = mir::graphics::kms;

namespace
{
char const* bypass_option_name{"bypass"};

}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const& device,
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mg::DisplayReport> const& report)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    // ensure atomic-kms finds the atomic-kms mir-platform symbols

    if (options->is_set(mir::options::debug_opt))
    {
        mg::initialise_egl_logger();
    }

    auto bypass_option = mga::BypassOption::allowed;
    if (!options->get<bool>(bypass_option_name))
        bypass_option = mga::BypassOption::prohibited;

    return mir::make_module_ptr<mga::Platform>(
        *device.device,
        report,
        *console,
        *emergency_cleanup_registry,
        bypass_option,
        mir::graphics::atomic::Quirks{*options}.gbm_quirks_for(*device.device));
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    config.add_options()
        (bypass_option_name,
         boost::program_options::value<bool>()->default_value(false),
         "[platform-specific] utilize the bypass optimization for fullscreen surfaces.");
    mga::Quirks::add_quirks_option(config);
}

namespace
{
class MinimalGLConfig : public mir::graphics::GLConfig
{
public:
    int depth_buffer_bits() const override
    {
        return 0;
    }

    int stencil_buffer_bits() const override
    {
        return 0;
    }
};
}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::udev::Context> const& udev,
    mir::options::Option const& options) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);

    mga::Quirks quirks{options};

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.scan_devices();

    if (drm_devices.begin() == drm_devices.end())
    {
        mir::log_info("Unsupported: No DRM devices detected");
        return {};
    }

    // We also require GBM EGL platform
    auto const* client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!client_extensions)
    {
        // Doesn't support EGL client extensions; Mesa does, so this is unlikely to be atomic-kms.
        mir::log_info("Unsupported: EGL platform does not support client extensions.");
        return {};
    }
    if (strstr(client_extensions, "EGL_KHR_platform_gbm") == nullptr)
    {
        // Doesn't support the Khronos-standardised GBM platform…
        mir::log_info("EGL platform does not support EGL_KHR_platform_gbm extension");
        // …maybe we support the old pre-standardised Mesa GBM platform?
        if (strstr(client_extensions, "EGL_MESA_platform_gbm") == nullptr)
        {
            mir::log_info(
                "Unsupported: EGL platform supports neither EGL_KHR_platform_gbm nor EGL_MESA_platform_gbm");
            return {};
        }
    }


    // Check for suitability
    std::vector<mg::SupportedDevice> supported_devices;
    mir::Fd tmp_fd;
    for (auto& device : drm_devices)
    {
        if (quirks.should_skip(device))
        {
            mir::log_info("Not probing device %s due to specified quirk", device.devnode());
            continue;
        }

        auto const devnum = device.devnum();
        if (devnum == makedev(0, 0))
        {
            /* The display connectors attached to the card appear as subdevices
             * of the card[0-9] node.
             * These won't have a device node, so pass on anything that doesn't have
             * a /dev/dri/card* node
             */
            continue;
        }

        try
        {
            // Rely on the console handing us a DRM master...
            auto const device_cleanup = console->acquire_device(
                major(devnum), minor(devnum),
                std::make_unique<mgc::OneShotDeviceObserver>(tmp_fd)).get();

            // We have a device. We don't know if it works yet, but it's a device we *could* support
            supported_devices.emplace_back(
                mg::SupportedDevice{
                    device.clone(),
                    mg::probe::unsupported,
                    nullptr
                });

            if (tmp_fd != mir::Fd::invalid)
            {
                // Check that the drm device is usable by setting the interface version we use (1.4)
                drmSetVersion sv;
                sv.drm_di_major = 1;
                sv.drm_di_minor = 4;
                sv.drm_dd_major = -1;     /* Don't care */
                sv.drm_dd_minor = -1;     /* Don't care */

                if (auto error = -drmSetInterfaceVersion(tmp_fd, &sv))
                {
                    BOOST_THROW_EXCEPTION((std::system_error{
                        error,
                        std::system_category(),
                        std::string{"Failed to set DRM interface version on device "} + device.devnode()}));
                }

                if (!mgk::get_cap_checked(tmp_fd, DRM_CLIENT_CAP_ATOMIC))
                {
                    mir::log_info("KMS device %s does not support Atomic KMS", device.devnode());
                    continue;
                }

                // For now, we *also* require our DisplayPlatform to support creating a HW EGL context                
                mga::helpers::GBMHelper atomic_device{tmp_fd};
                mga::helpers::EGLHelper egl{MinimalGLConfig()};

                egl.setup(atomic_device);

                egl.make_current();

                auto const renderer_string = reinterpret_cast<char const*>(glGetString(GL_RENDERER));
                if (!renderer_string)
                {
                    throw mg::gl_error(
                        "Probe failed to query GL renderer");
                }

                using namespace std::literals::string_literals;
                if ("llvmpipe"s == renderer_string)
                {
                    mir::log_info("KMS device only has associated software renderer: %s, device unsuitable", renderer_string);
                    supported_devices.back().support_level = mg::probe::unsupported;
                    continue;
                }

                /* Check if modesetting is supported on this DRM node
                 * This must be done after drmSetInterfaceVersion() as, for Hysterical Raisins,
                 * drmGetBusid() will return nullptr unless drmSetInterfaceVersion() has already been called
                 */
                auto const busid = std::unique_ptr<char, decltype(&drmFreeBusid)>{
                    drmGetBusid(tmp_fd),
                    &drmFreeBusid
                };

                if (!busid)
                {
                    mir::log_warning(
                        "Failed to query BusID for device %s; cannot check if KMS is available",
                        device.devnode());
                    supported_devices.back().support_level = mg::probe::supported;
                }
                else
                {
                    mg::kms::DRMModeResources kms_resources{tmp_fd};
                    switch (auto err = -drmCheckModesettingSupported(busid.get()))
                    {
                    case 0:
                        // We've got a DRM device that supports KMS. Let's see if it's got any output hardware!
                        if ((kms_resources.num_connectors() > 0) &&
                            (kms_resources.num_crtcs() > 0) &&
                            (kms_resources.num_encoders() > 0))
                        {
                            // It supports KMS *and* can drive at least one physical output! Top hole!
                            supported_devices.back().support_level = mg::probe::best;
                        }
                        else
                        {
                            mir::log_info("KMS support found, but device has no output hardware.");
                            mir::log_info("This is probably a render-only hybrid graphics device");
                        }
                        break;

                    case ENOSYS:
                        if (quirks.require_modesetting_support(device))
                        {
                            throw std::runtime_error{std::string{"Device "}+device.devnode()+" does not support KMS"};
                        }

                        [[fallthrough]];
                    case EINVAL:
                        mir::log_warning(
                            "Failed to detect whether device %s supports KMS, continuing with lower confidence",
                            device.devnode());
                        supported_devices.back().support_level = mg::probe::supported;
                        break;

                    default:
                        mir::log_warning("Unexpected error from drmCheckModesettingSupported(): %s (%i), "
                                         "but continuing anyway", strerror(err), err);
                        mir::log_warning("Please file a bug at "
                                         "https://github.com/canonical/mir/issues containing this message");
                        supported_devices.back().support_level = mg::probe::supported;
                    }
                }
            }
        }
        catch (std::exception const& e)
        {
            mir::log(
                mir::logging::Severity::informational,
                MIR_LOG_COMPONENT,
                std::current_exception(),
                "Failed to probe DRM device");
        }
    }

    return supported_devices;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:atomic-kms",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}

