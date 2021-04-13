/*
 * Copyright © 2015-2020 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/c_memory.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/display_configuration.h"
#include <mir/graphics/display_configuration_policy.h>
#include "mir/graphics/egl_error.h"
#include "mir/graphics/virtual_output.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/atomic_frame.h"
#include "mir/graphics/transformation.h"
#include "display_configuration.h"
#include "display.h"
#include "platform.h"
#include "display_buffer.h"
#include "../X11_resources.h"

#include <boost/throw_exception.hpp>
#include <algorithm>

#define MIR_LOG_COMPONENT "display"
#include "mir/log.h"

namespace mx=mir::X;
namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

namespace
{
auto get_pixel_size_mm(xcb_connection_t* conn) -> geom::SizeF
{
    auto const screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    return geom::SizeF{
        float(screen->width_in_millimeters) / screen->width_in_pixels,
        float(screen->height_in_millimeters) / screen->height_in_pixels};
}

class XGLContext : public mir::renderer::gl::Context
{
public:
    XGLContext(::Display* const x_dpy,
               std::shared_ptr<mg::GLConfig> const& gl_config,
               EGLContext const shared_ctx)
        : egl{*gl_config, x_dpy, shared_ctx}
    {
    }

    ~XGLContext() = default;

    void make_current() const override
    {
        egl.make_current();
    }

    void release_current() const override
    {
        egl.release_current();
    }

private:
    mgx::helpers::EGLHelper const egl;
};
}

mgx::X11Window::X11Window(xcb_connection_t* conn,
                          EGLDisplay egl_dpy,
                          geom::Size const size,
                          EGLConfig const egl_cfg)
    : conn{conn}
{
    auto const screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    EGLint vid;
    if (!eglGetConfigAttrib(egl_dpy, egl_cfg, EGL_NATIVE_VISUAL_ID, &vid))
        BOOST_THROW_EXCEPTION(mg::egl_error("Cannot get config attrib"));

    auto const colormap = xcb_generate_id(conn);
    xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, screen->root_visual);

    uint32_t const value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    uint32_t value_list[32];
    value_list[0] = 0; // background_pixel
    value_list[1] = 0; // border_pixel
    value_list[2] = XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_EXPOSURE         |
                    XCB_EVENT_MASK_KEY_PRESS        |
                    XCB_EVENT_MASK_KEY_RELEASE      |
                    XCB_EVENT_MASK_BUTTON_PRESS     |
                    XCB_EVENT_MASK_BUTTON_RELEASE   |
                    XCB_EVENT_MASK_FOCUS_CHANGE     |
                    XCB_EVENT_MASK_ENTER_WINDOW     |
                    XCB_EVENT_MASK_LEAVE_WINDOW     |
                    XCB_EVENT_MASK_POINTER_MOTION;
    value_list[3] = colormap;

    win = xcb_generate_id(conn);
    xcb_create_window(
        conn,
        XCB_COPY_FROM_PARENT,
        win,
        screen->root,
        0, 0,
        size.width.as_int(), size.height.as_int(),
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        value_mask,
        value_list);

    std::string const delete_win_str{"WM_DELETE_WINDOW"};
    auto const delete_win_cookie = xcb_intern_atom(conn, 0, delete_win_str.size(), delete_win_str.c_str());
    std::string const protocols_str{"WM_PROTOCOLS"};
    auto const protocols_cookie = xcb_intern_atom(conn, 0, protocols_str.size(), protocols_str.c_str());

    auto const delete_win_reply = make_unique_cptr(xcb_intern_atom_reply(conn, delete_win_cookie, nullptr));
    auto const protocols_reply = make_unique_cptr(xcb_intern_atom_reply(conn, protocols_cookie, nullptr));
    // Enable the WM_DELETE_WINDOW protocol for the window (causes a client message to be sent when window is closed)
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, (*protocols_reply).atom, 4, 32, 1, &(*delete_win_reply).atom);

    xcb_map_window(conn, win);
}

mgx::X11Window::~X11Window()
{
    xcb_destroy_window(conn, win);
}

mgx::X11Window::operator xcb_window_t() const
{
    return win;
}

unsigned long mgx::X11Window::red_mask() const
{
    return r_mask;
}

mgx::Display::Display(mir::X::X11Connection* x11_connection,
                      std::vector<X11OutputConfig> const& requested_sizes,
                      std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
                      std::shared_ptr<GLConfig> const& gl_config,
                      std::shared_ptr<DisplayReport> const& report)
    : shared_egl{*gl_config, x11_connection->xlib_dpy},
      x11_connection{x11_connection},
      gl_config{gl_config},
      pixel_size_mm{get_pixel_size_mm(x11_connection->conn)},
      report{report},
      last_frame{std::make_shared<AtomicFrame>()}
{
    geom::Point top_left{0, 0};

    for (auto const& requested_size : requested_sizes)
    {
        auto actual_size = requested_size.size;
        auto window = std::make_unique<X11Window>(
            x11_connection->conn,
            shared_egl.display(),
            actual_size,
            shared_egl.config());
        auto red_mask = window->red_mask();
        auto pf = (red_mask == 0xFF0000 ?
            mir_pixel_format_argb_8888 :
            mir_pixel_format_abgr_8888);
        auto configuration = DisplayConfiguration::build_output(
            pf,
            actual_size,
            top_left,
            geom::Size{
                actual_size.width * pixel_size_mm.width.as_value(),
                actual_size.height * pixel_size_mm.height.as_value()},
            requested_size.scale,
            mir_orientation_normal);
        auto display_buffer = std::make_unique<mgx::DisplayBuffer>(
            x11_connection->xlib_dpy,
            configuration->id,
            *window,
            configuration->extents(),
            shared_egl.context(),
            last_frame,
            report,
            *gl_config);
        top_left.x += as_delta(configuration->extents().size.width);
        outputs.push_back(std::make_unique<OutputInfo>(this, move(window), move(display_buffer), move(configuration)));
    }

    shared_egl.make_current();

    auto const display_config = configuration();
    initial_conf_policy->apply_to(*display_config);
    configure(*display_config);
    report->report_successful_display_construction();
    xcb_flush(x11_connection->conn);
}

mgx::Display::~Display() noexcept
{
}

void mgx::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f)
{
    std::lock_guard<std::mutex> lock{mutex};
    for (auto const& output : outputs)
    {
        f(*output->display_buffer);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mgx::Display::configuration() const
{
    std::lock_guard<std::mutex> lock{mutex};
    std::vector<DisplayConfigurationOutput> output_configurations;
    for (auto const& output : outputs)
    {
        output_configurations.push_back(*output->config);
    }
    return std::make_unique<mgx::DisplayConfiguration>(output_configurations);
}

void mgx::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    std::lock_guard<std::mutex> lock{mutex};

    if (!new_configuration.valid())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("Invalid or inconsistent display configuration"));
    }

    new_configuration.for_each_output([&](DisplayConfigurationOutput const& conf_output)
    {
        bool found_info = false;

        for (auto& output : outputs)
        {
            if (output->config->id == conf_output.id)
            {
                *output->config = conf_output;
                output->display_buffer->set_view_area(output->config->extents());
                output->display_buffer->set_transformation(output->config->transformation());
                found_info = true;
                break;
            }
        }

        if (!found_info)
            mir::log_error("Could not find info for output %d", conf_output.id.as_value());
    });
}

void mgx::Display::register_configuration_change_handler(
    EventHandlerRegister& /* event_handler*/,
    DisplayConfigurationChangeHandler const& change_handler)
{
    std::lock_guard<std::mutex> lock{mutex};
    config_change_handlers.push_back(change_handler);
}

void mgx::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mgx::Display::pause()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::pause()' not yet supported on x11 platform"));
}

void mgx::Display::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::resume()' not yet supported on x11 platform"));
}

auto mgx::Display::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    return nullptr;
}

std::unique_ptr<mg::VirtualOutput> mgx::Display::create_virtual_output(int /*width*/, int /*height*/)
{
    return nullptr;
}

std::unique_ptr<mir::renderer::gl::Context> mgx::Display::create_gl_context() const
{
    return std::make_unique<XGLContext>(x11_connection->xlib_dpy, gl_config, shared_egl.context());
}

bool mgx::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& /*conf*/)
{
    return false;
}

mg::Frame mgx::Display::last_frame_on(unsigned) const
{
    return last_frame->load();
}

mgx::Display::OutputInfo::OutputInfo(
    Display* owner,
    std::unique_ptr<X11Window> window,
    std::unique_ptr<DisplayBuffer> display_buffer,
    std::shared_ptr<DisplayConfigurationOutput> configuration)
    : owner{owner},
      window{move(window)},
      display_buffer{move(display_buffer)},
      config{move(configuration)}
{
    mx::X11Resources::instance.set_set_output_for_window(*this->window, this);
}

mgx::Display::OutputInfo::~OutputInfo()
{
    mx::X11Resources::instance.clear_output_for_window(*window);
}

void mgx::Display::OutputInfo::set_size(geometry::Size const& size)
{
    std::unique_lock<std::mutex> lock{owner->mutex};
    if (config->modes[0].size == size)
    {
        return;
    }
    config->modes[0].size = size;
    display_buffer->set_view_area({display_buffer->view_area().top_left, size});
    auto const handlers = owner->config_change_handlers;

    lock.unlock();
    for (auto const& handler : handlers)
    {
        handler();
    }
}
