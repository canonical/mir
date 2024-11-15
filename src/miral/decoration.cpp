/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/geometry/forward.h"
#include "miral/decoration.h"

#include "decoration_adapter.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/log.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/server.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/wayland/weak.h"
#include "miral/decoration_basic_manager.h"
#include "miral/decoration_manager_builder.h"

#include <memory>

namespace msh = mir::shell;
namespace msd = msh::decoration;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geometry = mir::geometry;
namespace geom = mir::geometry;

using DeviceEvent = msd::DeviceEvent;

struct DecorationRedrawNotifier
{
    using OnRedraw = std::function<void()>;
    void notify()
    {
        if (on_redraw)
            on_redraw();
    }

    void register_listener(OnRedraw on_redraw)
    {
        this->on_redraw = on_redraw;
    }

    OnRedraw on_redraw;
};

struct miral::Decoration::Self
{
    void render_titlebar(Renderer::Buffer titlebar_pixels, mir::geometry::Size scaled_titlebar_size)
    {
        for (geometry::Y y{0}; y < as_y(scaled_titlebar_size.height); y += geometry::DeltaY{1})
        {
            Renderer::render_row(titlebar_pixels, scaled_titlebar_size, {0, y}, scaled_titlebar_size.width, 0xFF00FFFF);
        }
    }

    DecorationRedrawNotifier& redraw_notifier() { return redraw_notifier_; }

    Self();

    struct InputManager
    {
        enum class ButtonState
        {
            Up,
            Hovered,
            Down,
        };

        struct Widget
        {
            Widget(MirResizeEdge resize_edge) :
                resize_edge{resize_edge}
            {
            }

            geometry::Rectangle rect;
            ButtonState state{ButtonState::Up};
            // mir_resize_edge_none is used to mean the widget moves the window
            std::optional<MirResizeEdge> const resize_edge;
        };

        auto widget_at(geom::Point location) -> std::optional<std::shared_ptr<Widget>>;

        void process_drag(DeviceEvent& device, InputContext ctx);
        void process_enter(DeviceEvent& device);
        void process_leave();
        void process_down();
        void process_up();

        void widget_drag(Widget& widget, InputContext ctx);
        void widget_leave(Widget& widget);
        void widget_enter(Widget& widget);
        void widget_down(Widget& widget);
        void widget_up(Widget& widget);

        void update_window_state(WindowState const& window_state);
        auto resize_edge_rect(WindowState const& window_state, MirResizeEdge resize_edge) -> geom::Rectangle;

        std::vector<std::shared_ptr<Widget>> widgets = {std::make_shared<Widget>(mir_resize_edge_none)};
        std::optional<std::shared_ptr<Widget>> active_widget;
    };

    DecorationRedrawNotifier redraw_notifier_;
    InputManager input_manager;
};

miral::Decoration::Self::Self() = default;
miral::Decoration::Decoration() :
    self{std::make_unique<Self>()}
{
}

auto miral::Decoration::Self::InputManager::widget_at(geom::Point location) -> std::optional<std::shared_ptr<Widget>>
{
    for (auto const& widget : widgets)
    {
        if (widget->rect.contains(location))
            return widget;
    }
    return std::nullopt;
}

void miral::Decoration::Self::InputManager::process_drag(DeviceEvent& device, InputContext ctx)
{
    auto const new_widget = widget_at(device.location);

    if (active_widget)
    {
        widget_drag(*active_widget.value(), ctx);
        if (new_widget != active_widget)
            widget_leave(*active_widget.value());
    }

    bool const enter = new_widget && (active_widget != new_widget);
    active_widget = new_widget;
    if (enter)
        widget_enter(*active_widget.value());
}

void miral::Decoration::Self::InputManager::process_enter(DeviceEvent& device)
{
    active_widget = widget_at(device.location);
    if (active_widget)
        widget_enter(*active_widget.value());
}

void miral::Decoration::Self::InputManager::process_leave()
{
    if (active_widget)
    {
        widget_leave(*active_widget.value());
        active_widget = std::nullopt;
    }
}

void miral::Decoration::Self::InputManager::process_down()
{
    if (active_widget)
    {
        widget_down(*active_widget.value());
    }
}

void miral::Decoration::Self::InputManager::process_up()
{
    if (active_widget)
    {
        widget_up(*active_widget.value());
    }
}

void miral::Decoration::Self::InputManager::widget_down(Widget& widget)
{
    widget.state = ButtonState::Down;
}

void miral::Decoration::Self::InputManager::widget_up(Widget& widget)
{
    widget.state = ButtonState::Hovered;
}

void miral::Decoration::Self::InputManager::widget_drag(Widget& widget, InputContext ctx)
{
    if (widget.state == ButtonState::Down)
    {
        if (widget.resize_edge)
        {
            auto edge = widget.resize_edge.value();

            if (edge == mir_resize_edge_none)
            {
                ctx.request_move();
            }
        }
        widget.state = ButtonState::Up;
    }

}

void miral::Decoration::Self::InputManager::widget_leave(Widget& widget) {
    widget.state = ButtonState::Up;
}

void miral::Decoration::Self::InputManager::widget_enter(Widget& widget)
{
    widget.state = ButtonState::Hovered;
}

void miral::Decoration::Self::InputManager::update_window_state(WindowState const& window_state)
{
    for (auto const& widget : widgets)
    {
        widget->rect = resize_edge_rect(window_state, widget->resize_edge.value());
    }
}

auto miral::Decoration::Self::InputManager::resize_edge_rect(
    WindowState const& window_state,
    MirResizeEdge resize_edge) -> geom::Rectangle
{
    switch (resize_edge)
    {
    case mir_resize_edge_none:
        return window_state.titlebar_rect();

    // TODO: the rest
    default: return {};
    }
}

auto create_surface(
    std::shared_ptr<ms::Surface> window_surface,
    std::shared_ptr<msh::Shell> shell) -> std::shared_ptr<ms::Surface>
{
    auto const& session = window_surface->session().lock();
    msh::SurfaceSpecification params;
    params.type = mir_window_type_decoration;
    params.parent = window_surface;
    auto const size = window_surface->window_size();
    params.width = size.width;
    params.height = size.height;
    params.aux_rect = {{}, {}};
    params.aux_rect_placement_gravity = mir_placement_gravity_northwest;
    params.surface_placement_gravity = mir_placement_gravity_northwest;
    params.placement_hints = MirPlacementHints(0);
    // Will be replaced by initial update
    params.streams = {{
        session->create_buffer_stream(mg::BufferProperties{
            geom::Size{1, 1},
            buffer_format,
            mg::BufferUsage::software}),
        {},
        }};
    return shell->create_surface(session, {}, params, nullptr, nullptr);
}

auto miral::Decoration::create_manager(mir::Server& server)
    -> std::shared_ptr<miral::DecorationManagerAdapter>
{
    return DecorationBasicManager(
               [server](auto shell, auto window_surface)
               {
                   auto session = window_surface->session().lock();
                   auto decoration_surface = create_surface(window_surface, shell);

                   // User code
                   auto decoration = std::make_shared<Decoration>();
                   auto decoration_adapter = std::make_shared<miral::DecorationAdapter>(
                       [decoration](Renderer::Buffer titlebar_pixels, geometry::Size scaled_titlebar_size)
                       {
                           decoration->self->render_titlebar(titlebar_pixels, scaled_titlebar_size);
                       },
                       [](auto...)
                       {
                       },
                       [](auto...)
                       {
                       },
                       [](auto...)
                       {
                       },
                       [decoration](auto... args)
                       {
                           // Author should figure out if changes require a redraw
                           // then call DecorationRedrawNotifier::notify()
                           // Here we just assume we're always redrawing
                           mir::log_debug("process_enter");
                           decoration->self->input_manager.process_enter(args...);
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_leave");
                           decoration->self->input_manager.process_leave();
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_down");
                           decoration->self->input_manager.process_down();
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_up");
                           decoration->self->input_manager.process_up();
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_move");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto... args)
                       {
                           mir::log_debug("process_drag");
                           decoration->self->input_manager.process_drag(args...);
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto window_state)
                       {
                           decoration->self->input_manager.update_window_state(*window_state);
                       });

                   // User code end

                   decoration_adapter->init(window_surface, decoration_surface, server.the_buffer_allocator(), shell);

                   // Rendering events come after this point
                   decoration->self->redraw_notifier().register_listener(
                       [decoration_adapter, decoration]()
                       {
                           decoration_adapter->update();
                       });

                   // Initial redraw to set up the margins and buffers for decorations.
                   // Just like BasicDecoration
                   decoration->self->redraw_notifier().notify();

                   return decoration_adapter;
               })
        .to_adapter();
}
