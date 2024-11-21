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
#include "mir/compositor/buffer_stream.h"
#include "mir/optional_value.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/server.h"
#include "mir/shell/decoration.h"
#include "mir/shell/decoration/basic_manager.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/wayland/weak.h"

#include "miral/decoration_adapter.h"
#include "decoration_manager_adapter.h"

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
            geometry::Size{1, 1},
            miral::decoration::Renderer::buffer_format,
            mg::BufferUsage::software}),
        {},
        }};
    return shell->create_surface(session, {}, params, nullptr, nullptr);
}

struct miral::DecorationBasicManager::Self : public mir::shell::decoration::BasicManager
{
    Self(mir::Server& server, Decoration::DecorationBuilder&& decoration_builder) :
        mir::shell::decoration::BasicManager(
            *server.the_display_configuration_observer_registrar(),
            [&server, decoration_builder](auto shell, auto window_surface)
            {
                auto session = window_surface->session().lock();
                auto decoration_surface = create_surface(window_surface, shell);
                auto decoration_adapter = decoration_builder();

                decoration_adapter->init(
                    window_surface,
                    decoration_surface,
                    server.the_buffer_allocator(),
                    shell,
                    server.the_cursor_images());

                // After this point, `DecorationAdapter` has done its job
                // (passing info from the user to the impl) and the the impl is
                // moved out of it
                return decoration_adapter->to_decoration();
            })
    {
    }

    void init(std::weak_ptr<mir::shell::Shell> const& shell)
    {
        this->shell = shell;
    }
};

miral::DecorationBasicManager::DecorationBasicManager(
    mir::Server& server, Decoration::DecorationBuilder&& decoration_builder) :
    self{std::make_shared<Self>(server, std::move(decoration_builder))}
{
}

auto miral::DecorationBasicManager::to_adapter() -> std::shared_ptr<DecorationManagerAdapter>
{
    auto adapter = std::shared_ptr<DecorationManagerAdapter>(new DecorationManagerAdapter());

    adapter->on_init = [self = this->self](auto shell)
    {
        self->init(shell);
    };
    adapter->on_decorate = [self = this->self](auto... args)
    {
        self->decorate(args...);
    };
    adapter->on_undecorate = [self = this->self](auto... args)
    {
        self->undecorate(args...);
    };
    adapter->on_undecorate_all = [self = this->self]()
    {
        self->undecorate_all();
    };

    return adapter;
}
