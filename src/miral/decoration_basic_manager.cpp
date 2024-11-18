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

#include "miral/decoration_basic_manager.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/server.h"
#include "mir/shell/decoration/basic_manager.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/optional_value.h"
#include "mir/wayland/weak.h"
#include "mir/compositor/buffer_stream.h"

#include "miral/decoration_adapter.h"
#include "miral/decoration_manager_builder.h"

#include <memory>
#include <utility>

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

struct miral::DecorationBasicManager::Self : public mir::shell::decoration::BasicManager
{
    Self(mir::Server& server, Decoration::DecorationBuilder&& decoration_builder) :
        mir::shell::decoration::BasicManager(
            [&server,decoration_builder](auto shell, auto window_surface)
            {
                auto session = window_surface->session().lock();
                auto decoration_surface = create_surface(window_surface, shell);

                auto decoration_adapter = decoration_builder(shell, window_surface);
                auto decoration = decoration_adapter->to_decoration();

                decoration_adapter->init(
                    window_surface,
                    decoration_surface,
                    server.the_buffer_allocator(),
                    shell,
                    server.the_cursor_images());

                // Rendering events come after this point
                decoration_adapter->redraw_notifier()->register_listener(
                    [decoration_adapter]()
                    {
                        decoration_adapter->update();
                    });

                // Initial redraw to set up the margins and buffers for decorations.
                // Just like BasicDecoration
                decoration_adapter->redraw_notifier()->notify();

                return decoration;
            })
    {
    }
};

miral::DecorationBasicManager::DecorationBasicManager(
    mir::Server& server, Decoration::DecorationBuilder&& decoration_builder) :
    self{std::make_shared<Self>(server, std::move(decoration_builder))}
{
}

auto miral::DecorationBasicManager::to_adapter() -> std::shared_ptr<DecorationManagerAdapter>
{
    return DecorationManagerBuilder::build(
        [self=this->self](auto... args) { self->init(args...); },
        [self=this->self](auto... args) { self->decorate(args...); },
        [self=this->self](auto... args) { self->undecorate(args...); },
        [self=this->self]() { self->undecorate_all(); }
    ).done();
}
