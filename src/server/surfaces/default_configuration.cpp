/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"

#include "surface_allocator.h"
#include "surface_stack.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/input/input_configuration.h"

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace msh = mir::shell;

std::shared_ptr<ms::SurfaceStackModel>
mir::DefaultServerConfiguration::the_surface_stack_model()
{
    return surface_stack(
        [this]()
        {
            auto const surfaces_report = the_surfaces_report();

            auto const factory = std::make_shared<ms::SurfaceAllocator>(
                the_buffer_stream_factory(),
                the_input_channel_factory(),
                surfaces_report);

            auto const ss = std::make_shared<ms::SurfaceStack>(
                factory,
                the_input_registrar(),
                surfaces_report);

            the_input_configuration()->set_input_targets(ss);

            return ss;
        });
}

std::shared_ptr<mc::Scene>
mir::DefaultServerConfiguration::the_scene()
{
    return surface_stack(
        [this]()
        {
            auto const surfaces_report = the_surfaces_report();

            auto const factory = std::make_shared<ms::SurfaceAllocator>(
                the_buffer_stream_factory(),
                the_input_channel_factory(),
                surfaces_report);

            auto const ss = std::make_shared<ms::SurfaceStack>(
                factory,
                the_input_registrar(),
                surfaces_report);

            the_input_configuration()->set_input_targets(ss);

            return ss;
        });
}


std::shared_ptr<msh::SurfaceBuilder>
mir::DefaultServerConfiguration::the_surface_builder()
{
    return the_surface_controller();
}

std::shared_ptr<ms::SurfaceController>
mir::DefaultServerConfiguration::the_surface_controller()
{
    return surface_controller(
        [this]()
        {
            return std::make_shared<ms::SurfaceController>(the_surface_stack_model());
        });
}

std::shared_ptr<msh::SurfaceController>
mir::DefaultServerConfiguration::the_shell_surface_controller()
{
    return the_surface_controller();
}
