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

    // TODO: make them const
    DecorationRedrawNotifier redraw_notifier_;
};

miral::Decoration::Self::Self() = default;
miral::Decoration::Decoration() :
    self{std::make_unique<Self>()}
{
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
                       [decoration](auto...)
                       {
                           // Author should figure out if changes require a redraw
                           // then call DecorationRedrawNotifier::notify()
                           // Here we just assume we're always redrawing
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
