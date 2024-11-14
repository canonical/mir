/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_DECORATION_DECORATION_ADAPTER_H
#define MIRAL_DECORATION_DECORATION_ADAPTER_H

/* #include "mir/shell/decoration_notifier.h" */

#include "mir/scene/surface.h"
#include "mir/shell/decoration.h"
#include "miral/decoration.h"
#include "mir/shell/decoration/input_resolver.h"

#include <functional>
#include <memory>

namespace msd = mir::shell::decoration;
namespace ms = mir::scene;

struct MirEvent;

namespace miral
{
class DecorationBuilder;

using DeviceEvent = msd::DeviceEvent;

struct InputResolverAdapter: public msd::InputResolver
{
    InputResolverAdapter(
        std::shared_ptr<ms::Surface> decoration_surface,
        std::function<void(DeviceEvent& device)> process_enter,
        std::function<void()> process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        std::function<void(DeviceEvent& device)> process_move,
        std::function<void(DeviceEvent& device)> process_drag) :
        msd::InputResolver(decoration_surface),
        on_process_enter{process_enter},
        on_process_leave{process_leave},
        on_process_down{process_down},
        on_process_up{process_up},
        on_process_move{process_move},
        on_process_drag{process_drag}
    {
    }

    void process_enter(DeviceEvent& device) override
    {
        on_process_enter(device);
    }
    void process_leave() override
    {
        on_process_leave();
    }
    void process_down() override
    {
        on_process_down();
    }
    void process_up() override
    {
        on_process_up();
    }
    void process_move(DeviceEvent& device) override
    {
        on_process_move(device);
    }
    void process_drag(DeviceEvent& device) override
    {
        on_process_drag(device);
    }

    std::function<void(DeviceEvent& device) > on_process_enter;
    std::function<void() > on_process_leave;
    std::function<void() > on_process_down;
    std::function<void() > on_process_up;
    std::function<void(DeviceEvent& device) > on_process_move;
    std::function<void(DeviceEvent& device) > on_process_drag;
};

class WindowSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    WindowSurfaceObserver(
        std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> attrib_changed,
        std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
            window_resized_to,
        std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> window_renamed) :
        on_attrib_changed{attrib_changed},
        on_window_resized_to(window_resized_to),
        on_window_renamed(window_renamed)
    {
    }

    /// Overrides from NullSurfaceObserver
    /// @{
    void attrib_changed(ms::Surface const* window_surface, MirWindowAttrib attrib, int value) override
    {
        switch(attrib)
        {
        case mir_window_attrib_type:
        case mir_window_attrib_state:
        case mir_window_attrib_focus:
        case mir_window_attrib_visibility:
            on_attrib_changed(window_surface, attrib, value);
            break;

        case mir_window_attrib_dpi:
        case mir_window_attrib_preferred_orientation:
        case mir_window_attribs:
            break;
        }
    }

    void window_resized_to(ms::Surface const* window_surface, mir::geometry::Size const& window_size) override
    {
        on_window_resized_to(window_surface, window_size);
    }

    void renamed(ms::Surface const* window_surface, std::string const& name) override
    {
        on_window_renamed(window_surface, name);
    }
    /// @}

private:

    std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> on_attrib_changed;
    std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
        on_window_resized_to;
    std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> on_window_renamed;
};


class DecorationAdapter : public mir::shell::decoration::Decoration
{
public:
    using Buffer = miral::Decoration::Buffer;
    using DeviceEvent = miral::Decoration::DeviceEvent;

    std::function<void(Buffer, mir::geometry::Size)> render_titlebar;
    std::function<void(Buffer, mir::geometry::Size)> render_left_border;
    std::function<void(Buffer, mir::geometry::Size)> render_right_border;
    std::function<void(Buffer, mir::geometry::Size)> render_bottom_border;

    std::function<void(DeviceEvent& device)> on_process_enter;
    std::function<void()> on_process_leave;
    std::function<void()> on_process_down;
    std::function<void()> on_process_up;
    std::function<void(DeviceEvent& device)> on_process_move;
    std::function<void(DeviceEvent& device)> on_process_drag;

    std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> on_attrib_changed;
    std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
        on_window_resized_to;
    std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> on_window_renamed;

    DecorationAdapter() = delete;

    DecorationAdapter(
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar,
        std::function<void(Buffer, mir::geometry::Size)> render_left_border,
        std::function<void(Buffer, mir::geometry::Size)> render_right_border,
        std::function<void(Buffer, mir::geometry::Size)> render_bottom_border,

        std::function<void(DeviceEvent& device)> process_enter,
        std::function<void()> process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        std::function<void(DeviceEvent& device)> process_move,
        std::function<void(DeviceEvent& device)> process_drag,
        std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> attrib_changed,
        std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
            window_resized_to,
        std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> window_renamed
) :
        render_titlebar{render_titlebar},
        render_left_border{render_left_border},
        render_right_border{render_right_border},
        render_bottom_border{render_bottom_border},
        on_process_enter{process_enter},
        on_process_leave{process_leave},
        on_process_down{process_down},
        on_process_up{process_up},
        on_process_move{process_move},
        on_process_drag{process_drag},
        on_attrib_changed{attrib_changed},
        on_window_resized_to(window_resized_to),
        on_window_renamed(window_renamed)
    {
    }

    void set_scale(float) override
    {
    }

    void init_input(std::shared_ptr<ms::Surface> decoration_surface)
    {
        input_adapter = std::make_unique<InputResolverAdapter>(
            decoration_surface,
            on_process_enter,
            on_process_leave,
            on_process_down,
            on_process_up,
            on_process_move,
            on_process_drag);
    }

    void init_window_surface_observer(std::shared_ptr<ms::Surface> window_surface)
    {
        this->window_surface = window_surface;

        window_surface_observer = std::shared_ptr<WindowSurfaceObserver>();
        window_surface->register_interest(window_surface_observer);
    }

    ~DecorationAdapter()
    {
        window_surface->unregister_interest(*window_surface_observer);
    }

    /* void handle_input_event(std::shared_ptr<MirEvent const> const& event) final override; */
    /* void set_scale(float new_scale) final override; */
    /* void attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value) final override; */
    /* void window_resized_to( */
    /*     mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size) final override; */
    /* void window_renamed(mir::scene::Surface const* window_surface, std::string const& name) final override; */

private:
    /* friend DecorationBuilder; */
    /* DecorationAdapter( */
    /*     std::shared_ptr<mir::scene::Surface> decoration_surface, std::shared_ptr<mir::scene::Surface> window_surface); */

    /* std::function<void(std::shared_ptr<MirEvent const> const&)> on_handle_input_event; */
    /* std::function<void(float new_scale)> on_set_scale; */
    /* std::function<void(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value)> on_attrib_changed; */
    /* std::function<void(mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size)> */
    /*     on_window_resized_to; */
    /* std::function<void(mir::scene::Surface const* window_surface, std::string const& name)> on_window_renamed; */

    /* mir::shell::decoration::DecorationNotifier decoration_notifier; */

    std::unique_ptr<InputResolverAdapter> input_adapter;
    std::shared_ptr<WindowSurfaceObserver> window_surface_observer;
    std::shared_ptr<ms::Surface> window_surface;
};

}
#endif
