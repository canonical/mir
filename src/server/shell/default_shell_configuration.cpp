/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/shell/default_shell_configuration.h"

#include "mir/shell/organising_surface_factory.h"
#include "mir/shell/consuming_placement_strategy.h"
#include "mir/shell/session_container.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/single_visibility_focus_mechanism.h"

namespace msh = mir::shell;
namespace mg = mir::graphics;

msh::DefaultShellConfiguration::DefaultShellConfiguration(std::shared_ptr<mg::ViewableArea> const& view_area,
                                                          std::shared_ptr<msh::InputFocusSelector> const& focus_selector,
                                                          std::shared_ptr<msh::SurfaceFactory> const& surface_factory)
  : view_area(view_area),
    input_focus_selector(focus_selector),
    underlying_surface_factory(surface_factory)
{
}

std::shared_ptr<msh::SurfaceFactory> msh::DefaultShellConfiguration::the_surface_factory()
{
    return surface_factory(
        [this]() -> std::shared_ptr<msh::SurfaceFactory>
        {
            return std::make_shared<msh::OrganisingSurfaceFactory>(underlying_surface_factory, the_placement_strategy());
        });
}

std::shared_ptr<msh::SessionContainer> msh::DefaultShellConfiguration::the_session_container()
{
    return session_container(
        [this]()
        {
            return std::make_shared<msh::SessionContainer>();
        });
}

std::shared_ptr<msh::FocusSequence> msh::DefaultShellConfiguration::the_focus_sequence()
{
    return focus_sequence(
        [this]()
        {
            return std::make_shared<msh::RegistrationOrderFocusSequence>(the_session_container());
        });                 
}

std::shared_ptr<msh::FocusSetter> msh::DefaultShellConfiguration::the_focus_setter()
{
    return focus_setter(
        [this]()
        {
            return std::make_shared<msh::SingleVisibilityFocusMechanism>(the_session_container(), input_focus_selector);
        });
}

std::shared_ptr<msh::PlacementStrategy> msh::DefaultShellConfiguration::the_placement_strategy()
{
    return placement_strategy(
        [this]()
        {
            return std::make_shared<msh::ConsumingPlacementStrategy>(view_area);
        });
}
