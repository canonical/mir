/*
 * Copyright © 2013-2017 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <mir/shell/system_compositor_window_manager.h>
#include "mir/default_server_configuration.h"
#include "null_host_lifecycle_event_listener.h"

#include "mir/input/composite_event_filter.h"
#include "mir/shell/abstract_shell.h"
#include "default_persistent_surface_store.h"
#include "frontend_shell.h"
#include "graphics_display_layout.h"
#include "decoration/basic_manager.h"
#include "decoration/decoration.h"

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;
namespace mf = mir::frontend;

auto mir::DefaultServerConfiguration::the_shell() -> std::shared_ptr<msh::Shell>
{
    return shell([this]
        {
            auto const result = wrap_shell(std::make_shared<msh::AbstractShell>(
                the_input_targeter(),
                the_surface_stack(),
                the_session_coordinator(),
                the_prompt_session_manager(),
                the_shell_report(),
                the_window_manager_builder(),
                the_seat(),
                the_decoration_manager()));

            the_composite_event_filter()->prepend(result);
            the_decoration_manager()->init(result);

            return result;
        });
}

auto mir::DefaultServerConfiguration::the_window_manager_builder() -> shell::WindowManagerBuilder
{
    return [this](msh::FocusController* focus_controller)
        { return std::make_shared<msh::DefaultWindowManager>(
            focus_controller,
            the_shell_display_layout(),
            the_session_coordinator()); };
}

auto mir::DefaultServerConfiguration::the_decoration_manager() -> std::shared_ptr<msd::Manager>
{
    return decoration_manager(
        []()->std::shared_ptr<msd::Manager>
        {
            return std::make_shared<msd::BasicManager>([](
                    std::shared_ptr<shell::Shell> const& /*shell*/,
                    std::shared_ptr<scene::Surface> const& /*surface*/) -> std::unique_ptr<msd::Decoration>
                {
                    return std::make_unique<msd::NullDecoration>();
                });
        });
}

auto mir::DefaultServerConfiguration::wrap_shell(std::shared_ptr<msh::Shell> const& wrapped) -> std::shared_ptr<msh::Shell>
{
    return wrapped;
}

std::shared_ptr<msh::PersistentSurfaceStore>
mir::DefaultServerConfiguration::the_persistent_surface_store()
{
    return persistent_surface_store([]()
    {
        return std::make_shared<msh::DefaultPersistentSurfaceStore>();
    });
}

std::shared_ptr<mf::Shell>
mir::DefaultServerConfiguration::the_frontend_shell()
{
    return frontend_shell([this]
        {
            return std::make_shared<msh::detail::FrontendShell>(
                the_shell(),
                the_persistent_surface_store(),
                the_display(),
                the_display_configuration_observer_registrar());
        });
}

std::shared_ptr<msh::FocusController>
mir::DefaultServerConfiguration::the_focus_controller()
{
    return the_shell();
}

std::shared_ptr<msh::DisplayLayout>
mir::DefaultServerConfiguration::the_shell_display_layout()
{
    return shell_display_layout(
        [this]()
        {
            return std::make_shared<msh::GraphicsDisplayLayout>(the_display());
        });
}

std::shared_ptr<msh::HostLifecycleEventListener>
mir::DefaultServerConfiguration::the_host_lifecycle_event_listener()
{
    return host_lifecycle_event_listener(
        []()
        {
            return std::make_shared<msh::NullHostLifecycleEventListener>();
        });
}
