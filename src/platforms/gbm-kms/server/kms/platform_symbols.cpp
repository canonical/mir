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
 */

#define MIR_LOG_COMPONENT "gbm-kms"
#include "mir/log.h"

#include "platform.h"
#include "gbm_platform.h"
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
namespace mgg = mg::gbm;
namespace mgc = mir::graphics::common;
namespace mo = mir::options;

namespace
{
char const* bypass_option_name{"bypass"};

}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const&,
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
    mg::SupportedDevice const& device,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const& displays,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    return mir::make_module_ptr<mgg::RenderingPlatform>(*device.device, displays);
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

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::udev::Context> const& udev,
    mir::options::ProgramOption const& options) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);

    mgg::Quirks quirks{options};

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
        // Doesn't support EGL client extensions; Mesa does, so this is unlikely to be gbm-kms.
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
                    mg::PlatformPriority::unsupported,
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
                    throw std::system_error{
                        error,
                        std::system_category(),
                        std::string{"Failed to set DRM interface version on device "} + device.devnode()};
                }

                // For now, we *also* require our DisplayPlatform to support creating a HW EGL context                
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

                using namespace std::literals::string_literals;
                if ("llvmpipe"s == renderer_string)
                {
                    mir::log_info("KMS device only has associated software renderer: %s, device unsuitable", renderer_string);
                    supported_devices.back().support_level = mg::PlatformPriority::unsupported;
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
                    supported_devices.back().support_level = mg::PlatformPriority::supported;
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
                            supported_devices.back().support_level = mg::PlatformPriority::best;
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
                        supported_devices.back().support_level = mg::PlatformPriority::supported;
                        break;

                    default:
                        mir::log_warning("Unexpected error from drmCheckModesettingSupported(): %s (%i), "
                                         "but continuing anyway", strerror(err), err);
                        mir::log_warning("Please file a bug at "
                                         "https://github.com/MirServer/mir/issues containing this message");
                        supported_devices.back().support_level = mg::PlatformPriority::supported;
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

auto probe_rendering_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::ProgramOption const& options) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_rendering_platform);

    mgg::Quirks quirks{options};
    auto udev = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.match_sysname("renderD[0-9]*");
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
        // Doesn't support EGL client extensions; Mesa does, so this is unlikely to be gbm-kms.
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
            // Open the device node directly; we don't need DRM master
            mir::Fd tmp_fd{::open(device_node, O_RDWR | O_CLOEXEC)};

            /* We prefer render nodes for the RenderingPlatform.
             * Check if this device has an associated render node,
             * and skip it if it does.
             */
            std::unique_ptr<char, std::function<void(char*)>> const render_node{
                drmGetRenderDeviceNameFromFd(tmp_fd),
                &free
            };
            if (render_node)
            {
                // We might already be probing a render node
                if (strcmp(device_node, render_node.get()))
                {
                    // The render node of this device is not *this* device,
                    // so let's pass and let us open the render node later.
                    continue;
                }
            }

            // We know we've got a device that we *might* be able to use
            // Mark it as “supported” because we should *at least* be able to get a software context on it.
            supported_devices.emplace_back(mg::SupportedDevice{device.clone(), mg::PlatformPriority::supported, nullptr});
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
                    // Leave the priority at ::supported; if we've got a software renderer then
                    // we *don't* support *this* device very well.
                }
                else
                {
                    // We've got a non-software renderer. That's our cue!
                    supported_devices.back().support_level = mg::PlatformPriority::best;
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
