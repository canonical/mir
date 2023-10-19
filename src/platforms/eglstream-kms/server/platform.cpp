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
#include "buffer_allocator.h"
#include "display.h"
#include "mir/graphics/platform.h"
#include "utils.h"
#include "eglstream_interface_provider.h"
#include "basic_cpu_addressable_display_provider.h"

#include "one_shot_device_observer.h"

#include "mir/console_services.h"
#include "mir/graphics/egl_error.h"
#include "mir/renderer/gl/context.h"

#include <sys/sysmacros.h>
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_file_name.hpp>

namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;
namespace mge = mir::graphics::eglstream;

namespace
{
const auto mir_xwayland_option = "MIR_XWAYLAND_OPTION";

auto make_egl_context(EGLDisplay dpy) -> EGLContext
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_STREAM_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig egl_config;
    EGLint num_egl_configs{0};

    if (eglChooseConfig(dpy, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose ARGB EGL config"));
    }

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLContext const ctx = eglCreateContext(dpy, egl_config, EGL_NO_CONTEXT, context_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGL context")));
    }
    return ctx;
}

class BasicEGLContext : public mir::renderer::gl::Context
{
public:
    BasicEGLContext(EGLDisplay dpy)
        : dpy{dpy},
          ctx{make_egl_context(dpy)}
    {
    }

    BasicEGLContext(EGLDisplay dpy, EGLContext copy_from)
        : dpy{dpy},
          ctx{duplicate_context(dpy, copy_from)}
    {
    }

    void make_current() const override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make EGL context current"));
        }
    }

    void release_current() const override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release current EGL context"));
        }
    }

    auto make_share_context() const -> std::unique_ptr<Context> override
    {
        return std::make_unique<BasicEGLContext>(dpy, ctx);
    }

    explicit operator EGLContext() override
    {
        return ctx;
    }

private:
    static auto get_context_attrib(EGLDisplay dpy, EGLContext ctx, EGLenum attrib) -> EGLint
    {
        EGLint result;
        if (eglQueryContext(dpy, ctx, attrib, &result) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query attribute of EGLContext"));
        }
        return result;
    }

    static auto duplicate_context(EGLDisplay dpy, EGLContext copy_from) -> EGLContext
    {
        EGLint const api = get_context_attrib(dpy, copy_from, EGL_CONTEXT_CLIENT_TYPE);
        std::optional<EGLint> client_version;
        if (api == EGL_OPENGL_ES_API)
        {
            // Client version only exists for GLES contexts
            client_version = get_context_attrib(dpy, copy_from, EGL_CONTEXT_CLIENT_VERSION);
        }

        EGLint const config_id = get_context_attrib(dpy, copy_from, EGL_CONFIG_ID);
        EGLConfig config;
        int num_configs;
        EGLint const attributes[] = {
            EGL_CONFIG_ID, config_id,
            EGL_NONE
        };
        if (eglChooseConfig(dpy, attributes, &config, 1, &num_configs) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to find existing EGLContext's EGLConfig")));
        }

        std::vector<EGLint> ctx_attribs;
        if (client_version)
        {
            ctx_attribs.push_back(EGL_CONTEXT_CLIENT_VERSION);
            ctx_attribs.push_back(client_version.value());
        }
        ctx_attribs.push_back(EGL_NONE);
        eglBindAPI(api);
        auto ctx = eglCreateContext(dpy, config, copy_from, ctx_attribs.data());
        if (ctx == EGL_NO_CONTEXT)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create duplicate context"));
        }
        return ctx;
    }

    EGLDisplay const dpy;
    EGLContext const ctx;
};
}

mge::RenderingPlatform::RenderingPlatform(EGLDisplay dpy)
    : dpy{dpy},
      ctx{std::make_unique<BasicEGLContext>(dpy)}
{
    // XWayland eglstream has always been kinda flaky, now it's somehow worse.
    // Disable it until we've had a chance to look at what's wrong.
    // setenv(mir_xwayland_option, "-eglstream", 1);
}

mge::RenderingPlatform::~RenderingPlatform()
{
    unsetenv(mir_xwayland_option);
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mge::RenderingPlatform::create_buffer_allocator(
    mg::Display const&)
{
    return mir::make_module_ptr<mge::BufferAllocator>(ctx->make_share_context());
}

auto mge::RenderingPlatform::maybe_create_provider(
    RenderingProvider::Tag const& type_tag) -> std::shared_ptr<RenderingProvider>
{
    if (dynamic_cast<graphics::GLRenderingProvider::Tag const*>(&type_tag))
    {
        return std::make_shared<mge::GLRenderingProvider>(dpy, ctx->make_share_context());
    }
    return nullptr;
}

mge::DisplayPlatform::DisplayPlatform(
    ConsoleServices& console,
    EGLDeviceEXT device,
    std::shared_ptr<mg::DisplayReport> display_report)
    : display_report{std::move(display_report)}
{
    using namespace std::literals;

    auto const devnum = devnum_for_device(device);
    drm_device = console.acquire_device(
        major(devnum), minor(devnum),
        std::make_unique<mgc::OneShotDeviceObserver>(drm_node))
            .get();

    if (drm_node == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to acquire DRM device node for device"}));
    }

    int const drm_node_attrib[] = {
        EGL_DRM_MASTER_FD_EXT, static_cast<int>(drm_node), EGL_NONE
    };
    display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, device, drm_node_attrib);

    if (display == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGLDisplay on EGLDeviceEXT")));
    }

    std::tuple<EGLint, EGLint> egl_version = std::make_tuple(1, 4);
    if (eglInitialize(display, &std::get<0>(egl_version), &std::get<1>(egl_version)) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to initialize EGL")));
    }
    if (egl_version < std::make_tuple(1, 4))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Incompatible EGL version"s +
            "Wanted 1.4, got " +
            std::to_string(std::get<0>(egl_version)) + "." + std::to_string(std::get<1>(egl_version))}));
    }

    provider = std::make_shared<InterfaceProvider>(display, drm_node);
}

mge::DisplayPlatform::~DisplayPlatform()
{
    if (display != EGL_NO_DISPLAY)
    {
        eglTerminate(display);
    }
}

auto mge::DisplayPlatform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
    std::shared_ptr<GLConfig> const& gl_config)
    -> UniqueModulePtr<mg::Display>
{
    return mir::make_module_ptr<mge::Display>(
        provider,
        drm_node,
        display,
        configuration_policy,
        *gl_config,
        display_report);
}

auto mge::DisplayPlatform::interface_for() -> std::shared_ptr<DisplayInterfaceProvider>
{
    return provider;
}
