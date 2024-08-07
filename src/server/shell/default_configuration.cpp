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

#include <boost/lexical_cast.hpp>
#include <mir/shell/system_compositor_window_manager.h>
#include <mir/main_loop.h>
#include "mir/default_server_configuration.h"
#include "mir/abnormal_exit.h"

#include "mir/input/composite_event_filter.h"
#include "mir/shell/abstract_shell.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "default_persistent_surface_store.h"
#include "graphics_display_layout.h"
#include "decoration/basic_manager.h"
#include "decoration/basic_decoration.h"
#include "basic_idle_handler.h"

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
        [this]()->std::shared_ptr<msd::Manager>
        {
            auto options = the_options();
            auto x11_scale = boost::lexical_cast<float>(options->get<std::string>(options::x11_scale_opt));
            return std::make_shared<msd::BasicManager>(
                *the_display_configuration_observer_registrar(),
                [buffer_allocator = the_buffer_allocator(),
                 executor = the_main_loop(),
                 cursor_images = the_cursor_images(),
                 x11_scale](
                    std::shared_ptr<shell::Shell> const& shell,
                    std::shared_ptr<scene::Surface> const& surface) -> std::unique_ptr<msd::Decoration>
                {
                    return std::make_unique<msd::BasicDecoration>(
                        shell,
                        buffer_allocator,
                        executor,
                        cursor_images,
                        surface,
                        x11_scale);
                });
        });
}

auto mir::DefaultServerConfiguration::wrap_shell(std::shared_ptr<msh::Shell> const& wrapped) -> std::shared_ptr<msh::Shell>
{
    return wrapped;
}

auto mir::DefaultServerConfiguration::the_idle_handler() -> std::shared_ptr<msh::IdleHandler>
{
    return idle_handler([this]()
        {
            auto const idle_handler = std::make_shared<msh::BasicIdleHandler>(
                the_idle_hub(),
                the_input_scene(),
                the_buffer_allocator(),
                the_display_configuration_controller());

            auto options = the_options();
            int const idle_timeout_seconds = options->get<int>(options::idle_timeout_opt);
            if (idle_timeout_seconds < 0)
            {
                throw mir::AbnormalExit(
                    "Invalid " +
                    std::string{options::idle_timeout_opt} +
                    " value " +
                    std::to_string(idle_timeout_seconds) +
                    ", must be > 0");
            }
            else if (idle_timeout_seconds == 0)
            {
                idle_handler->set_display_off_timeout(std::nullopt);
            }
            else
            {
                idle_handler->set_display_off_timeout(std::chrono::seconds{idle_timeout_seconds});
            }

            return idle_handler;
        });
}

std::shared_ptr<msh::PersistentSurfaceStore>
mir::DefaultServerConfiguration::the_persistent_surface_store()
{
    return persistent_surface_store([]()
    {
        return std::make_shared<msh::DefaultPersistentSurfaceStore>();
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
