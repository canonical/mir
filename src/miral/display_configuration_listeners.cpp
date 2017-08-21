/*
 * Copyright © 2017 Canonical Ltd.
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

#include "display_configuration_listeners.h"

#include "miral/active_outputs.h"

#include <mir/graphics/display_configuration.h>

#include <algorithm>

void miral::DisplayConfigurationListeners::add_listener(ActiveOutputsListener* listener)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    active_output_listeners.push_back(listener);
}

void miral::DisplayConfigurationListeners::delete_listener(ActiveOutputsListener* listener)
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    auto const new_end = std::remove(active_output_listeners.begin(), active_output_listeners.end(), listener);
    active_output_listeners.erase(new_end, active_output_listeners.end());
}

void miral::DisplayConfigurationListeners::process_active_outputs(
    std::function<void(std::vector<Output> const& outputs)> const& functor) const
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    functor(active_outputs);
}

void miral::DisplayConfigurationListeners::initial_configuration(std::shared_ptr<mir::graphics::DisplayConfiguration const> const& configuration)
{
    configuration_applied(configuration);
}

void miral::DisplayConfigurationListeners::configuration_failed(
    std::shared_ptr<mir::graphics::DisplayConfiguration const> const&,
    std::exception const&)
{
}

void miral::DisplayConfigurationListeners::catastrophic_configuration_error(
    std::shared_ptr<mir::graphics::DisplayConfiguration const> const&,
    std::exception const&)
{
}

void miral::DisplayConfigurationListeners::base_configuration_updated(std::shared_ptr<mir::graphics::DisplayConfiguration const> const& ) {}

void miral::DisplayConfigurationListeners::session_configuration_applied(std::shared_ptr<mir::frontend::Session> const&,
                                   std::shared_ptr<mir::graphics::DisplayConfiguration> const&) {}

void miral::DisplayConfigurationListeners::session_configuration_removed(std::shared_ptr<mir::frontend::Session> const&) {}

void miral::DisplayConfigurationListeners::configuration_applied(std::shared_ptr<mir::graphics::DisplayConfiguration const> const& config)
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    decltype(active_outputs) current_outputs;

    for (auto const l : active_output_listeners)
        l->advise_output_begin();

    config->for_each_output(
        [&current_outputs, this](mir::graphics::DisplayConfigurationOutput const& output)
            {
            Output o{output};

            if (!o.connected() || !o.valid()) return;

            auto op = find_if(
                begin(active_outputs), end(active_outputs), [&](Output const& oo)
                    { return oo.is_same_output(o); });

            if (op == end(active_outputs))
            {
                for (auto const l : active_output_listeners)
                    l->advise_output_create(o);
            }
            else if (!equivalent_display_area(o, *op))
            {
                for (auto const l : active_output_listeners)
                    l->advise_output_update(o, *op);
            }

            current_outputs.push_back(o);
            });

    for (auto const& o : active_outputs)
    {
        auto op = find_if(
            begin(current_outputs), end(current_outputs), [&](Output const& oo)
                { return oo.is_same_output(o); });

        if (op == end(current_outputs))
            for (auto const l : active_output_listeners)
                l->advise_output_delete(o);
    }

    current_outputs.swap(active_outputs);
    for (auto const l : active_output_listeners)
        l->advise_output_end();
}
