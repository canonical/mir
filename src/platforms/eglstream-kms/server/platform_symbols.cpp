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

#include <epoxy/egl.h>

#include "platform.h"
#include "utils.h"
#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/options/configuration.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/log.h"
#include "mir/graphics/egl_error.h"
#include "one_shot_device_observer.h"
#include "mir/raii.h"
#include "kms-utils/drm_mode_resources.h"
#include "mir/graphics/egl_logger.h"
#include "mir/udev/wrapper.h"

#include <boost/throw_exception.hpp>
#include <xf86drm.h>
#include <sstream>
#include <variant>

#include <fcntl.h>
#include <sys/sysmacros.h>
#include <boost/exception/errinfo_file_name.hpp>
#include <xf86drmMode.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mgc = mir::graphics::common;
namespace mge = mir::graphics::eglstream;

auto create_rendering_platform(
    mg::SupportedDevice const& device,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const& /*displays*/,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    auto platform_data = std::any_cast<std::variant<EGLDisplay, EGLDeviceEXT>>(device.platform_data);

    EGLDisplay display{EGL_NO_DISPLAY};

    if (auto dpy = std::get_if<0>(&platform_data))
    {
        display = *dpy;
    }
    else if (auto device = std::get_if<1>(&platform_data))
    {
        display = eglGetPlatformDisplayEXT(
            EGL_PLATFORM_DEVICE_EXT,
            *device,
            nullptr);

        if (display == EGL_NO_DISPLAY)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGL Display")));
        }

        EGLint major_ver{1}, minor_ver{4};
        if (!eglInitialize(display, &major_ver, &minor_ver))
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to initialise EGL")));
        }
    }

    return mir::make_module_ptr<mge::RenderingPlatform>(display);
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

auto probe_rendering_platform(
    std::span<std::shared_ptr<mg::DisplayPlatform>> const& displays,
    mir::ConsoleServices& /*console*/,
    std::shared_ptr<mir::udev::Context> const& udev,
    mo::Option const& /*options*/) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::RenderProbe>(&probe_rendering_platform);

    mg::probe::Result maximum_suitability = mg::probe::unsupported;
    // First check if there are any displays we can possibly drive
    std::vector<std::shared_ptr<mg::EGLStreamDisplayProvider>> eglstream_providers;
    for (auto const& display_provider : displays)
    {
        if (auto provider = display_provider->acquire_provider<mg::EGLStreamDisplayProvider>())
        {
            // We can optimally drive an EGLStream display
            mir::log_debug("EGLStream-capable display found");
            maximum_suitability = mg::probe::best;
            eglstream_providers.push_back(provider);
        }
        if (display_provider->acquire_provider<mg::CPUAddressableDisplayProvider>())
        {
            /* We *can* support this output, but with slower buffer copies
             * If another platform supports this device better, let it.
             */
            maximum_suitability = mg::probe::supported;
        }
    }

    if (maximum_suitability == mg::probe::unsupported)
    {
        mir::log_debug("No outputs capable of accepting EGLStream input detected");
        mir::log_debug("Probing will be skipped");
        return {};
    }

    std::vector<char const*> missing_extensions;
    for (char const* extension : {
        "EGL_EXT_platform_base",
        "EGL_EXT_platform_device",
        "EGL_EXT_device_base",
        "EGL_EXT_device_enumeration",
        "EGL_EXT_device_query"})
    {
        if (!epoxy_has_egl_extension(EGL_NO_DISPLAY, extension))
        {
            missing_extensions.push_back(extension);
        }
    }

    if (!missing_extensions.empty())
    {
        std::stringstream message;
        message << "Missing required extension" << (missing_extensions.size() > 1 ? "s:" : ":");
        for (auto missing_extension : missing_extensions)
        {
            message << " " << missing_extension;
        }

        mir::log_debug("EGLStream platform is unsupported: %s",
                       message.str().c_str());
        return {};
    }

    int device_count{0};
    if (eglQueryDevicesEXT(0, nullptr, &device_count) != EGL_TRUE)
    {
        mir::log_info("Platform claims to support EGL_EXT_device_base, but "
                      "eglQueryDevicesEXT falied: %s",
                      mg::egl_category().message(eglGetError()).c_str());
        return {};
    }

    auto devices = std::make_unique<EGLDeviceEXT[]>(device_count);
    if (eglQueryDevicesEXT(device_count, devices.get(), &device_count) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get device list with eglQueryDevicesEXT"));
    }

    std::vector<mg::SupportedDevice> supported_devices;
    for (auto i = 0; i != device_count; ++i)
    {
        auto const& device = devices[i];
        try
        {
            supported_devices.emplace_back(mg::SupportedDevice{
                udev->char_device_from_devnum(mge::devnum_for_device(device)),
                mg::probe::unsupported,
                nullptr
            });
        }
        catch (std::exception const& e)
        {
            mir::log_debug("Failed to find kernel device for EGLDevice: %s", e.what());
            continue;
        }

        EGLDisplay display{EGL_NO_DISPLAY};
        bool using_display_platform_dpy = false;
        for (auto const& display_provider : eglstream_providers)
        {
            auto display_dpy = display_provider->get_egl_display();
            EGLAttrib display_device;
            if (eglQueryDisplayAttribEXT(display_dpy, EGL_DEVICE_EXT, &display_device) == EGL_TRUE)
            {
                if (reinterpret_cast<EGLDeviceEXT>(display_device) == device)
                {
                    mir::log_debug("Rendering platform using EGLDeviceEXT matching Display platform");
                    display = display_dpy;
                    using_display_platform_dpy = true;
                }
            }
            else
            {
                mir::log_info("Failed to query EGLDeviceEXT from display platform");
            }
        }

        if (display == EGL_NO_DISPLAY)
        {
            display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, device, nullptr);

            if (display == EGL_NO_DISPLAY)
            {
                mir::log_debug("Failed to create EGLDisplay: %s", mg::egl_category().message(eglGetError()).c_str());
                continue;
            }
        }
        auto egl_init = mir::raii::paired_calls(
            [&display, using_display_platform_dpy]()
            {
                if (!using_display_platform_dpy)
                {
                    EGLint major_ver{1}, minor_ver{4};
                    if (!eglInitialize(display, &major_ver, &minor_ver))
                    {
                        mir::log_debug("Failed to initialise EGL: %s", mg::egl_category().message(eglGetError()).c_str());
                        display = EGL_NO_DISPLAY;
                    }
                }
            },
            [&display, using_display_platform_dpy]()
            {
                if (display != EGL_NO_DISPLAY && ! using_display_platform_dpy)
                {
                    eglTerminate(display);
                }
            });

        if (display != EGL_NO_DISPLAY)
        {
            std::vector<char const*> missing_extensions;
            for (char const* extension : {
                "EGL_KHR_stream_consumer_gltexture",
                "EGL_NV_stream_attrib"})
            {
                if (!epoxy_has_egl_extension(display, extension))
                {
                    missing_extensions.push_back(extension);
                }
            }

            for (auto const missing_extension: missing_extensions)
            {
                mir::log_info("EGLDevice found but unsuitable. Missing extension %s", missing_extension);
            }

            if (missing_extensions.empty())
            {
                // We've got EGL, and we've got the necessary EGL extensions. We're good.
                supported_devices.back().support_level = maximum_suitability;
                if (using_display_platform_dpy)
                {
                    supported_devices.back().platform_data = std::variant<EGLDisplay, EGLDeviceEXT>{
                        std::in_place_index<0>, display};
                }
                else
                {
                    supported_devices.back().platform_data = std::variant<EGLDisplay, EGLDeviceEXT>{
                        std::in_place_index<1>, device};
                }
            }
        }
    }
    if (!std::any_of(
        supported_devices.begin(),
        supported_devices.end(),
        [](auto const& device)
        {
            return device.support_level > mg::probe::unsupported;
        }))
    {
        mir::log_debug(
            "EGLDeviceEXTs found, but none are suitable for Mir");
    }

    return supported_devices;
}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const& device,
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mg::DisplayReport> const& display_report)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);

    if (options->is_set(mo::debug_opt))
    {
        mg::initialise_egl_logger();
    }

    return mir::make_module_ptr<mge::DisplayPlatform>(
        *console,
        std::any_cast<EGLDeviceEXT>(device.platform_data),
        display_report);
}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::udev::Context> const& udev,
    mo::Option const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);

    std::vector<char const*> missing_extensions;
    for (char const* extension : {
        "EGL_EXT_platform_base",
        "EGL_EXT_platform_device",
        "EGL_EXT_device_base",})
    {
        if (!epoxy_has_egl_extension(EGL_NO_DISPLAY, extension))
        {
            missing_extensions.push_back(extension);
        }
    }

    if (!missing_extensions.empty())
    {
        std::stringstream message;
        message << "Missing required extension" << (missing_extensions.size() > 1 ? "s:" : ":");
        for (auto missing_extension : missing_extensions)
        {
            message << " " << missing_extension;
        }

        mir::log_debug("EGLStream platform is unsupported: %s",
                       message.str().c_str());
        return {};
    }

    int device_count{0};
    if (eglQueryDevicesEXT(0, nullptr, &device_count) != EGL_TRUE)
    {
        mir::log_info("Platform claims to support EGL_EXT_device_base, but "
                      "eglQueryDevicesEXT falied: %s",
                      mg::egl_category().message(eglGetError()).c_str());
        return {};
    }

    auto devices = std::make_unique<EGLDeviceEXT[]>(device_count);
    if (eglQueryDevicesEXT(device_count, devices.get(), &device_count) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get device list with eglQueryDevicesEXT"));
    }

    std::vector<mg::SupportedDevice> supported_devices;
    for (auto i = 0; i != device_count; ++i)
    {
        auto const& device = devices[i];
        auto device_extensions = eglQueryDeviceStringEXT(device, EGL_EXTENSIONS);
        if (device_extensions)
        {
            mir::log_debug("Found EGLDeviceEXT with device extensions: %s",
                           device_extensions);
            // TODO: This test is not strictly correct (will incorrectly match
            // EGL_EXT_device_drmish_but_not_drm)
            if (strstr(device_extensions, "EGL_EXT_device_drm") != NULL)
            {
                // Check we can acquire the device...
                mir::Fd drm_fd;
                std::unique_ptr<mir::Device> device_holder;
                try
                {
                    auto const devnum = mge::devnum_for_device(device);

                    device_holder = console->acquire_device(
                        major(devnum), minor(devnum),
                        std::make_unique<mgc::OneShotDeviceObserver>(drm_fd)).get();

                    supported_devices.emplace_back(
                        mg::SupportedDevice{
                            udev->char_device_from_devnum(devnum),
                            mg::probe::unsupported,
                            device
                        });
                }
                catch (std::exception const& e)
                {
                    mir::log_info("Failed to query DRM node for EGLDevice: %s", e.what());
                    continue;
                }
                if (drm_fd == mir::Fd::invalid)
                {
                    mir::log_debug(
                        "EGL_EXT_device_drm found, but can't acquire DRM node.");
                    continue;
                }

                // Check that the drm device is usable by setting the interface version we use (1.4)
                drmSetVersion sv;
                sv.drm_di_major = 1;
                sv.drm_di_minor = 4;
                sv.drm_dd_major = -1;     /* Don't care */
                sv.drm_dd_minor = -1;     /* Don't care */

                if (auto error = -drmSetInterfaceVersion(drm_fd, &sv))
                {
                    mir::log_warning(
                        "Failed to set DRM interface version on device: %i (%s)",
                        error,
                        strerror(error));
                    continue;
                }

                auto busid = std::unique_ptr<char, decltype(&drmFreeBusid)>{
                    drmGetBusid(drm_fd),
                    &drmFreeBusid
                };
                if (auto err = drmCheckModesettingSupported(busid.get()))
                {
                    if (err == -ENOSYS)
                    {
                        mir::log_info("EGL_EXT_device_drm found, but no KMS support");
                        mir::log_info("You may need to set the nvidia_drm.modeset kernel parameter");
                    }
                    else
                    {
                        mir::log_warning(
                            "Failed to check DRM modesetting support for device %s: %s (%i)",
                            busid.get(),
                            strerror(-err),
                            -err);
                    }
                    continue;
                }

                mg::kms::DRMModeResources kms_resources{drm_fd};
                if (
                    kms_resources.num_connectors() == 0 ||
                    kms_resources.num_encoders() == 0 ||
                    kms_resources.num_crtcs() == 0)
                {
                    mir::log_info("KMS support found, but device has no output hardware.");
                    mir::log_info("This is probably a render-only hybrid graphics device");
                    continue;
                }

                EGLDisplay display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, device, nullptr);

                if (display == EGL_NO_DISPLAY)
                {
                    mir::log_debug("Failed to create EGLDisplay: %s", mg::egl_category().message(eglGetError()).c_str());
                    continue;
                }

                auto egl_init = mir::raii::paired_calls(
                    [&display]()
                    {
                        EGLint major_ver{1}, minor_ver{4};
                        if (!eglInitialize(display, &major_ver, &minor_ver))
                        {
                            mir::log_debug("Failed to initialise EGL: %s", mg::egl_category().message(eglGetError()).c_str());
                            display = EGL_NO_DISPLAY;
                        }
                    },
                    [&display]()
                    {
                        if (display != EGL_NO_DISPLAY)
                        {
                            eglTerminate(display);
                        }
                    });

                if (display == EGL_NO_DISPLAY)
                {
                    continue;
                }

                eglBindAPI(EGL_OPENGL_ES_API);
                EGLint const config_attribs[] = {
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_SURFACE_TYPE, EGL_STREAM_BIT_KHR,
                    EGL_NONE
                };
                EGLConfig config;
                EGLint num_configs;
                if (eglChooseConfig(display, config_attribs, &config, 1, &num_configs) != EGL_TRUE)
                {
                    mir::log_warning("Failed to create EGL context: no EGL_STREAM_BIT_KHR configs supported");
                    continue;
                }
                EGLContext ctx{EGL_NO_CONTEXT};
                auto ctx_init = mir::raii::paired_calls(
                    [&ctx, display, config]()
                    {
                        EGLint const context_attr[] = {
                            EGL_CONTEXT_CLIENT_VERSION, 2,
                            EGL_NONE
                        };
                        ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attr);
                    },
                    [&ctx, display]()
                    {
                        if (ctx != EGL_NO_CONTEXT)
                        {
                            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                            eglDestroyContext(display, ctx);
                        }
                    });

                if (ctx == EGL_NO_CONTEXT)
                {
                    mir::log_warning("Failed to create EGL context: %s", mg::egl_category().message(eglGetError()).c_str());
                    continue;
                }

                eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);

                auto const gl_version = reinterpret_cast<char const*>(glGetString(GL_VERSION));
                if (!gl_version)
                {
                    mir::log_warning("glGetString(GL_VERSION) call failed. This probably indicates a problem with the GL drivers.");
                }
                else if (auto version = mge::parse_nvidia_version(gl_version))
                {
                    mir::log_debug("Detected NVIDIA driver version %i.%i", version->major, version->minor);
                    if (version->major < 396)
                    {
                        mir::log_warning(
                            "Detected NVIDIA driver version %i.%i is older than 396.xx",
                            version->major,
                            version->minor);
                        mir::log_warning(
                            "This driver is known to interact badly with Mir. See https://github.com/MirServer/mir/issues/650");
                        mir::log_warning(
                            "Mir will not auto-load the eglstream-kms platform on this driver. To proceed anyway, manually specify the platform library.");
                        continue;
                    }
                }

                if (epoxy_has_egl_extension(display, "EGL_EXT_output_base"))
                {
                    supported_devices.back().support_level = mg::probe::best;
                }
                else
                {
                    mir::log_debug("EGL_EXT_device_drm found, but missing EGL_EXT_output_base extension.");
                    mir::log_debug("Available extensions are: %s", eglQueryString(display, EGL_EXTENSIONS));
                }
            }
        }
        else
        {
            mir::log_debug("Found EGLDeviceEXT with no device extensions");
        }
    }
    if (supported_devices.empty())
    {
        mir::log_debug(
            "EGLDeviceEXTs found, but none are suitable for Mir");
    }

    return supported_devices;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:eglstream-kms",
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

