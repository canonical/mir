/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <bcm_host.h>
/*
 * vcos_types.h “helpfully” #defines countof
 * Sadly, glm also implements glm::countof, with hilarious consequences.
 *
 * No Mir code cares about finding the count of a statically sized C-style array,
 * so…
 */
#undef countof

#include "display.h"
#include "display_buffer.h"

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/virtual_output.h"
#include "mir/renderer/gl/context.h"
#include "mir/signal_blocker.h"

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;

class mg::rpi::Display::DisplayConfiguration : public mg::DisplayConfiguration
{
public:
    explicit DisplayConfiguration(DISPMANX_DISPLAY_HANDLE_T const display)
        : the_output{query_modeinfo(display)}
    {
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const override
    {
        mg::DisplayConfigurationCard const the_card{mg::DisplayConfigurationCardId{1}, 1};
        f(the_card);
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        f(the_output);
    }
    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f) override
    {
        mg::UserDisplayConfigurationOutput user{the_output};
        f(user);
    }

    auto clone() const -> std::unique_ptr<mg::DisplayConfiguration> override
    {
        return std::unique_ptr<DisplayConfiguration>{
            new DisplayConfiguration{the_output}
        };
    }

private:
    static auto query_modeinfo(DISPMANX_DISPLAY_HANDLE_T display) -> mg::DisplayConfigurationOutput
    {
        DISPMANX_MODEINFO_T info;
        if (auto const error = vc_dispmanx_display_get_info(display, &info))
        {
            BOOST_THROW_EXCEPTION(
                (std::runtime_error{std::string{"Failed to query display info :"} + std::to_string(error)}));
        }

        mg::DisplayConfigurationOutput output;

        output.id = mg::DisplayConfigurationOutputId{1};
        output.card_id = mg::DisplayConfigurationCardId{1};
        output.type = mg::DisplayConfigurationOutputType::hdmia;
        output.pixel_formats = std::vector<MirPixelFormat>{input_format_to_pixel_format(info.input_format)};
        output.modes = std::vector<mg::DisplayConfigurationMode>{mg::DisplayConfigurationMode{
            geometry::Size{info.width, info.height},
            0  // Um, we don't get this info!
        }};
        output.preferred_mode_index = 0;
        output.connected = true;
        output.current_mode_index = 0;
        output.current_format = output.pixel_formats[0];
        output.power_mode = MirPowerMode::mir_power_mode_on;
        // TODO: orientation from mode.transform
        output.gamma_supported = MirOutputGammaSupported::mir_output_gamma_unsupported;
        output.form_factor = MirFormFactor::mir_form_factor_unknown;
        output.orientation = mir_orientation_normal;
        output.scale = 1.0f;
        output.subpixel_arrangement = mir_subpixel_arrangement_unknown;
        output.used = true;

        return output;
    }

    static auto input_format_to_pixel_format(DISPLAY_INPUT_FORMAT_T format) -> MirPixelFormat
    {
        switch (format)
        {
        case DISPLAY_INPUT_FORMAT_RGB888:
            return mir_pixel_format_rgb_888;
        case DISPLAY_INPUT_FORMAT_RGB565:
            return mir_pixel_format_rgb_565;
        case DISPLAY_INPUT_FORMAT_INVALID:
            return mir_pixel_format_rgb_888;
        }
#ifndef __clang__
        BOOST_THROW_EXCEPTION((std::logic_error{"Unhandled DISPLAY_INPUT_FORMAT type!"}));
#endif
    }

    explicit DisplayConfiguration(mg::DisplayConfigurationOutput from)
        : the_output{std::move(from)}
    {
    }

    mg::DisplayConfigurationOutput the_output;
};

namespace
{
EGLConfig choose_config(EGLDisplay display, mg::GLConfig const& requested_config)
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         0,
        EGL_DEPTH_SIZE,         requested_config.depth_buffer_bits(),
        EGL_STENCIL_SIZE,       requested_config.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE,    MIR_SERVER_EGL_OPENGL_BIT,
        EGL_NONE};

    EGLint num_egl_configs;
    EGLConfig egl_config;
    if (eglChooseConfig(display, config_attr, &egl_config, 1, &num_egl_configs) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to chose EGL config"));
    }
    else if (num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find compatible EGL config"});
    }

    return egl_config;
}

namespace
{
EGLint const client_version_2_if_gles_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
    EGL_CONTEXT_CLIENT_VERSION,
    2,
#endif
    EGL_NONE
};
}

EGLContext create_context(EGLDisplay display, EGLConfig config)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, client_version_2_if_gles_attr);
    if (context == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }

    return context;
}

EGLContext create_context(EGLDisplay display, EGLConfig config, EGLContext shared_context)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    EGLContext context = eglCreateContext(display, config, shared_context, client_version_2_if_gles_attr);
    if (context == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }

    return context;
}
}


mg::rpi::Display::Display(EGLDisplay dpy, GLConfig const& gl_config, uint32_t device)
    : disp_handle{vc_dispmanx_display_open(device)},
      dpy{dpy},
      egl_config{choose_config(dpy, gl_config)},
      ctx{create_context(dpy, egl_config)},
      display_config{std::make_unique<DisplayConfiguration>(disp_handle)},
      db{std::make_unique<rpi::DisplayBuffer>(size_from_config(*display_config), disp_handle, dpy, egl_config, ctx)}
{
}

mg::rpi::Display::~Display() noexcept
{
    vc_dispmanx_display_close(disp_handle);
}

void mg::rpi::Display::for_each_display_sync_group(std::function<void(DisplaySyncGroup&)> const& f)
{
    f(*db);
}

auto mg::rpi::Display::configuration() const
    -> std::unique_ptr<mg::DisplayConfiguration>
{
    return display_config->clone();
}

auto mg::rpi::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& conf)
    -> bool
{
    configure(conf);
    return true;
}

void mg::rpi::Display::configure(mg::DisplayConfiguration const& conf)
{
    conf.for_each_output(
        [](mg::DisplayConfigurationOutput const& output)
        {
            if (!output.used || output.power_mode != MirPowerMode::mir_power_mode_on)
                BOOST_THROW_EXCEPTION((std::runtime_error{"Turning off the display is not implemented"}));
        });

    display_config = std::unique_ptr<DisplayConfiguration>{
        dynamic_cast<DisplayConfiguration*>(conf.clone().release())};
}

void mg::rpi::Display::register_configuration_change_handler(
    mg::EventHandlerRegister&,
    mg::DisplayConfigurationChangeHandler const&)
{
}

void mg::rpi::Display::register_pause_resume_handlers(
    mg::EventHandlerRegister&,
    mg::DisplayPauseHandler const&,
    mg::DisplayResumeHandler const&)
{
}

void mg::rpi::Display::pause()
{
}

void mg::rpi::Display::resume()
{
}

auto mg::rpi::Display::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    // TODO: Should we implement this, or should we fix the server so that HW cursor is just
    // an automatically-enabled optimisation when there's a HW plane that can contain it?

    return {};
}

auto mg::rpi::Display::create_virtual_output(int /*width*/, int /*height*/) -> std::unique_ptr<VirtualOutput>
{
    // TODO: Is this an interface we need
    // TODO: Implement this; dispmanx is perfectly happy to do offscreen rendering
    return {nullptr};
}

auto mg::rpi::Display::native_display() -> NativeDisplay*
{
    return this;
}

auto mg::rpi::Display::last_frame_on(unsigned /*output_id*/) const -> Frame
{
    return {};
}

namespace
{
EGLint const dummy_pbuffer_attribs[] {
    EGL_WIDTH,  1,
    EGL_HEIGHT, 1,
    EGL_NONE
};
}

auto mg::rpi::Display::create_gl_context() const -> std::unique_ptr<renderer::gl::Context>
{
    class RpiGLContext : public renderer::gl::Context
    {
    public:
        RpiGLContext(EGLDisplay display, EGLConfig config, EGLContext shared_context)
            : dpy{display},
              ctx{create_context(display, config, shared_context)},
              dummy_surface{eglCreatePbufferSurface(dpy, config, dummy_pbuffer_attribs)}
        {
        }

        ~RpiGLContext() override
        {
            release_current();
            eglDestroySurface(dpy, dummy_surface);
            eglDestroyContext(dpy, ctx);
        }

        void make_current() const override
        {
            if (eglMakeCurrent(dpy, dummy_surface, dummy_surface, ctx) != EGL_TRUE)
            {
                BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
            }
        }

        void release_current() const override
        {
            if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
            {
                BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release context"));
            }
        }

    private:
        EGLDisplay const dpy;
        EGLContext const ctx;
        EGLSurface const dummy_surface;
    };

    return std::make_unique<RpiGLContext>(dpy, egl_config, ctx);
}

auto mg::rpi::Display::size_from_config(DisplayConfiguration const& config) -> geometry::Size
{
    geometry::Size size;
    config.for_each_output(
        [&size](DisplayConfigurationOutput const& output)
        {
            size = output.extents().size;
        });
    return size;
}
