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
#include "mir/observer_registrar.h"
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

StaticGeometry const default_geometry{
    geom::Height{24},         // titlebar_height
    geom::Width{6},           // side_border_width
    geom::Height{6},          // bottom_border_height
    geom::Size{16, 16},       // resize_corner_input_size
    geom::Width{24},          // button_width
    geom::Width{6},           // padding_between_buttons
    geom::Height{14},         // title_font_height
    geom::Point{8, 2},        // title_font_top_left
    geom::Displacement{5, 5}, // icon_padding
    geom::Width{1},           // detail_line_width
};

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
    void render_titlebar(Buffer titlebar_pixels, mir::geometry::Size scaled_titlebar_size)
    {
        for (geometry::Y y{0}; y < as_y(scaled_titlebar_size.height); y += geometry::DeltaY{1})
        {
            Renderer::render_row(titlebar_pixels, scaled_titlebar_size, {0, y}, scaled_titlebar_size.width, 0xFF00FFFF);
        }
    }

    void window_state_updated(ms::Surface const* window_surface)
    {
        window_state = std::make_shared<WindowState>(default_geometry, window_surface);
    }

    DecorationRedrawNotifier& redraw_notifier() { return redraw_notifier_; }

    Self(std::shared_ptr<ms::Surface> window_surface);

    std::shared_ptr<WindowState> window_state;

    // TODO: make them const
    std::shared_ptr<ms::Surface> decoration_surface;
    std::shared_ptr<ms::Surface> window_surface;
    DecorationRedrawNotifier redraw_notifier_;
};

miral::Decoration::Self::Self(std::shared_ptr<ms::Surface> window_surface) :
    window_state{std::make_unique<WindowState>(default_geometry, window_surface.get())}
{
}

miral::Decoration::Decoration(
    std::shared_ptr<mir::scene::Surface> window_surface) :
    self{std::make_unique<Self>(window_surface)}
{
}


auto miral::Decoration::window_state() -> std::shared_ptr<WindowState>
{
    return self->window_state;
}

auto miral::Decoration::update_state() -> msh::SurfaceSpecification
{
    auto& window_state = self->window_state;
    auto& window_surface = self->window_surface;

    msh::SurfaceSpecification spec;

    window_surface->set_window_margins(
        as_delta(window_state->titlebar_height()),
        geom::DeltaX{},
        geom::DeltaY{},
        geom::DeltaX{});

    if (window_state->window_size().width.as_value())
        spec.width = window_state->window_size().width;
    if (window_state->window_size().height.as_value())
        spec.height = window_state->window_size().height;

    spec.input_shape = { window_state->titlebar_rect() };

    return spec;
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
                   auto decoration = std::make_shared<Decoration>(window_surface);
                   auto decoration_surface = create_surface(window_surface, shell);
                   auto decoration_adapter = std::make_shared<miral::DecorationAdapter>(
                       [decoration](Decoration::Buffer titlebar_pixels, geometry::Size scaled_titlebar_size)
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
                       [decoration](auto...)
                       {
                           mir::log_debug("process_enter");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_leave");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_down");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_up");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_move");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto...)
                       {
                           mir::log_debug("process_drag");
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto window_surface, auto...)
                       {
                           decoration->window_state_updated(window_surface);
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto window_surface, auto...)
                       {
                           decoration->window_state_updated(window_surface);
                           decoration->self->redraw_notifier().notify();
                       },
                       [decoration](auto window_surface, auto...)
                       {
                           decoration->window_state_updated(window_surface);
                           decoration->self->redraw_notifier().notify();
                       });

                   decoration_adapter->init(window_surface, decoration_surface, server.the_buffer_allocator(), shell);

                   decoration->self->window_surface = window_surface;
                   decoration->self->decoration_surface = decoration_surface;

                   decoration->self->redraw_notifier().register_listener(
                       [decoration_adapter, decoration]()
                       {
                           decoration_adapter->update(decoration);
                       });

                   decoration->self->redraw_notifier().notify();

                   return decoration_adapter;
               })
        .to_adapter();
}

void miral::Decoration::window_state_updated(ms::Surface const* window_surface)
{
    self->window_state_updated(window_surface);
}

// On any of the process_* functions or window state updates
// I need to call DecorationAdapter::update.
//
// The key here is to take care of all rendering preparation for the user.
