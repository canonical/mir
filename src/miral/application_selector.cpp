/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "application_selector.h"
#include <miral/application_info.h>
#include <mir/scene/session.h>
#include <mir/log.h>

using namespace miral;

ApplicationSelector::ApplicationSelector(const miral::WindowManagerTools& in_tools)
    : tools{in_tools}
{}

auto ApplicationSelector::next(bool reverse) -> Application
{
    // First, we find the currently focused application
    if (!is_active())
    {
        auto active_window = tools.active_window();
        if (!active_window)
        {
            return nullptr;
        }

        auto application = active_window.application();
        if (!application)
        {
            return nullptr;
        }

        root = application;
        selected = root;
    }

    // Then, we find a suitable application in the list based off of the direction.
    // If we encounter theApplicationSelectorTest.run_in_circle presently selected application again, that means
    // we have gone all the way around the list, so we can "break".
    auto next_selected = selected;
    do {
        next_selected = reverse ? tools.get_previous_application(next_selected)
                                : tools.get_next_application(next_selected);
        if (next_selected == selected) break;
    } while (!tools.can_focus_application(next_selected));

    // Next, raise the window, but note that we do not actually focus it.
    // Note: The null checks here are mostly to accommodate the tests, however they are a good idea.
    auto surface = next_selected ? next_selected->default_surface() : nullptr;
    if (surface)
    {
        auto window = tools.info_for(surface).window();
        tools.raise_tree(window);
    }
    else
    {
        mir::log_warning("ApplicationSelector::raise_next: Failed to raise the newly selected window.");
    }

    selected = next_selected;
    return selected;
}

auto ApplicationSelector::complete() -> Application
{
    if (!is_active())
    {
        mir::log_warning("Cannot call ApplicationSelector::stop when the ApplicationSelector is not active. Call ApplicationSelector::start first.");
        return nullptr;
    }

    if (!try_select_application(selected))
    {
        mir::log_warning("ApplicationSelector::complete: Failed to select the active window.");
    }

    root = nullptr;
    return selected;
}

void ApplicationSelector::cancel()
{
    if (!try_select_application(root))
    {
        mir::log_warning("ApplicationSelector::cancel: Failed to select the root.");
    }
    
    root = nullptr;
    selected = nullptr;
}

auto ApplicationSelector::is_active() -> bool
{
    return root != nullptr;
}

auto ApplicationSelector::try_select_application(Application application) -> bool
{
    if (!application)
    {
        return false;
    }

    if (!tools.can_focus_application(application))
    {
        return false;
    }

    auto surface = application->default_surface();
    if (!surface)
    {
        return false;
    }

    auto window = tools.info_for(surface).window();
    tools.select_active_window(window);
    return true;
}