/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/set_window_management_policy.h"
#include "basic_window_manager.h"
#include "window_management_trace.h"

#include <mir/server.h>
#include <mir/options/option.h>
#include <mir/version.h>

namespace msh = mir::shell;

namespace
{
char const* const trace_option = "window-management-trace";
}

miral::SetWindowManagementPolicy::SetWindowManagementPolicy(WindowManagementPolicyBuilder const& builder) :
    builder{builder}
{
}

miral::SetWindowManagementPolicy::~SetWindowManagementPolicy() = default;

void miral::SetWindowManagementPolicy::operator()(mir::Server& server) const
{
    server.add_configuration_option(trace_option, "log trace message", mir::OptionType::null);

    server.override_the_window_manager_builder([this, &server](msh::FocusController* focus_controller)
        -> std::shared_ptr<msh::WindowManager>
        {
            auto const display_layout = server.the_shell_display_layout();

            auto const persistent_surface_store = server.the_persistent_surface_store();

            if (server.get_options()->is_set(trace_option))
            {
                auto trace_builder = [this](WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
                    {
                        return std::make_unique<WindowManagementTrace>(tools, builder);
                    };

                return std::make_shared<BasicWindowManager>(
                    focus_controller,
                    display_layout,
                    persistent_surface_store,
                    *server.the_display_configuration_observer_registrar(),
                    trace_builder);
            }

            return std::make_shared<BasicWindowManager>(
                focus_controller,
                display_layout,
                persistent_surface_store,
                *server.the_display_configuration_observer_registrar(),
                builder);
        });
}
