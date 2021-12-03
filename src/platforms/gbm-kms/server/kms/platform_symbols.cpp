/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#define MIR_LOG_COMPONENT "gbm-kms"
#include "mir/log.h"

#include "platform.h"
#include "gbm_platform.h"
#include "display_helpers.h"
#include "quirks.h"
#include "mir/options/program_option.h"
#include "mir/options/option.h"
#include "mir/options/configuration.h"
#include "mir/udev/wrapper.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/console_services.h"
#include "one_shot_device_observer.h"
#include "mir/raii.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_logger.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "egl_helper.h"
#include <fcntl.h>
#include <xf86drm.h>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;
namespace mgc = mir::graphics::common;
namespace mo = mir::options;

namespace
{
char const* bypass_option_name{"bypass"};
char const* host_socket{"host-socket"};

}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mg::DisplayReport> const& report)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    // ensure gbm-kms finds the gbm-kms mir-platform symbols

    if (options->is_set(mir::options::debug_opt))
    {
        mg::initialise_egl_logger();
    }

    auto bypass_option = mgg::BypassOption::allowed;
    if (!options->get<bool>(bypass_option_name))
        bypass_option = mgg::BypassOption::prohibited;

    auto quirks = std::make_unique<mgg::Quirks>(*options);

    return mir::make_module_ptr<mgg::Platform>(
        report, console, *emergency_cleanup_registry, bypass_option, std::move(quirks));
}

auto create_rendering_platform(
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    return mir::make_module_ptr<mgg::RenderingPlatform>();
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    config.add_options()
        (bypass_option_name,
         boost::program_options::value<bool>()->default_value(false),
         "[platform-specific] utilize the bypass optimization for fullscreen surfaces.");
    mgg::Quirks::add_quirks_option(config);
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

mg::PlatformPriority probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    mo::ProgramOption const& options)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);

    auto nested = options.is_set(host_socket);
    mgg::Quirks quirks{options};

    auto udev = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.scan_devices();

    if (drm_devices.begin() == drm_devices.end())
    {
        mir::log_info("Unsupported: No DRM devices detected");
        return mg::PlatformPriority::unsupported;
    }

    // We also require GBM EGL platform
    auto const* client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!client_extensions)
    {
        // Doesn't support EGL client extensions; Mesa does, so this is unlikely to be gbm-kms.
        mir::log_info("Unsupported: EGL platform does not support client extensions.");
        return mg::PlatformPriority::unsupported;
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
            return mg::PlatformPriority::unsupported;
        }
    }

    // Check for suitability
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
            auto maximum_suitability = mg::PlatformPriority::best;

            // Rely on the console handing us a DRM master...
            auto const device_cleanup = console->acquire_device(
                major(devnum), minor(devnum),
                std::make_unique<mgc::OneShotDeviceObserver>(tmp_fd)).get();

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
                    throw std::system_error{
                        error,
                        std::system_category(),
                        std::string{"Failed to set DRM interface version on device "} + device.devnode()};
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
                    maximum_suitability = mg::PlatformPriority::supported;
                }
                else
                {
                    switch (auto err = -drmCheckModesettingSupported(busid.get()))
                    {
                    case 0: break;

                    case ENOSYS:
                        if (getenv("MIR_MESA_KMS_DISABLE_MODESET_PROBE") == nullptr)
                        {
                            throw std::runtime_error{std::string{"Device "}+device.devnode()+" does not support KMS"};
                        }

                        mir::log_debug("MIR_MESA_KMS_DISABLE_MODESET_PROBE is set");
                        // Falls through.
                    case EINVAL:
                        mir::log_warning(
                            "Failed to detect whether device %s supports KMS, continuing with lower confidence",
                            device.devnode());
                        maximum_suitability = mg::PlatformPriority::supported;
                        break;

                    default:
                        mir::log_warning("Unexpected error from drmCheckModesettingSupported(): %s (%i), "
                                         "but continuing anyway", strerror(err), err);
                        mir::log_warning("Please file a bug at "
                                         "https://github.com/MirServer/mir/issues containing this message");
                        maximum_suitability = mg::PlatformPriority::supported;
                    }
                }

                return maximum_suitability;
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

    if (nested)
        return mg::PlatformPriority::supported;

    /*
     * We haven't found any suitable devices. We've complained above about
     * the reason, so we don't have to pretend to be supported.
     */
    return mg::PlatformPriority::unsupported;
}

mg::PlatformPriority probe_rendering_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    mir::options::ProgramOption const& options)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_rendering_platform);

    mgg::Quirks quirks{options};
    auto udev = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.match_sysname("render[0-9]*");
    drm_devices.scan_devices();

    if (drm_devices.begin() == drm_devices.end())
    {
        mir::log_info("Unsupported: No DRM devices detected");
        return mg::PlatformPriority::unsupported;
    }

    // We also require GBM EGL platform
    auto const* client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!client_extensions)
    {
        // Doesn't support EGL client extensions; Mesa does, so this is unlikely to be gbm-kms.
        mir::log_info("Unsupported: EGL platform does not support client extensions.");
        return mg::PlatformPriority::unsupported;
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
            return mg::PlatformPriority::unsupported;
        }
    }

    // Check for suitability
    auto maximum_suitability = mg::PlatformPriority::unsupported;
    for (auto& device : drm_devices)
    {
        if (quirks.should_skip(device))
        {
            mir::log_info("Not probing device %s due to specified quirk", device.devnode());
            continue;
        }

        auto const device_node = device.devnode();
        if (device_node == nullptr)
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
            auto const devnum = device.devnum();
            mir::Fd tmp_fd;

            // We should be able to simply open the relevant device node; DRM master is not
            // required for rendering, and is entirely inapplicable to rendernodes.
            // However, there's at least one EGL implementation supporting KHR_platform_gbm that requires
            // a DRM master fd for some reason, so let's try and get one…
            std::unique_ptr<mir::Device> device_cleanup{nullptr};
            try
            {
                device_cleanup = console->acquire_device(
                    major(devnum), minor(devnum),
                    std::make_unique<mgc::OneShotDeviceObserver>(tmp_fd)).get();
            }
            catch (std::exception const&)
            {
                //…ok, so we can't get a DRM master. Maybe we can just ::open() it?
                tmp_fd = mir::Fd{::open(device_node, O_RDWR | O_CLOEXEC)};
            }

            if (tmp_fd != mir::Fd::invalid)
            {
                mgg::helpers::GBMHelper gbm_device{tmp_fd};
                mgg::helpers::EGLHelper egl{MinimalGLConfig()};

                egl.setup(gbm_device);

                egl.make_current();

                auto const renderer_string = reinterpret_cast<char const*>(glGetString(GL_RENDERER));
                if (!renderer_string)
                {
                    throw mg::gl_error(
                        "Probe failed to query GL renderer");
                }

                // TODO: Probe for necessary EGL extensions (ie: at least one of WL_bind_display / dmabuf_import

                if (strncmp(
                    "llvmpipe",
                    renderer_string,
                    strlen("llvmpipe")) == 0)
                {
                    mir::log_info("Detected software renderer: %s", renderer_string);
                    maximum_suitability = mg::PlatformPriority::supported;
                }
                else
                {
                    // If we're good on *any* device, we're best.
                    return mg::PlatformPriority::best;
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

    return maximum_suitability;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:gbm-kms",
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
