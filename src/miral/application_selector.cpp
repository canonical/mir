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

#include <miral/application_selector.h>
#include <miral/application_info.h>
#include <miral/application.h>
#include <mir/log.h>
#include <iostream>

using namespace miral;

ApplicationSelector::ApplicationSelector(const miral::WindowManagerTools& in_tools)
    : tools{in_tools}
{}

void ApplicationSelector::advise_new_app(Application const& application)
{
    focus_list.push_back(application);
}

void ApplicationSelector::advise_focus_gained(WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        return;
    }

    auto application = window.application();
    if (!application)
    {
        return;
    }

    auto it = std::find(focus_list.begin(), focus_list.end(), application);
    if (!is_active())
    {
        // If we are not active, we move the newly focused item to the front of the list.
        if (it != focus_list.end())
        {
            std::rotate(focus_list.begin(), it, it + 1);
        }
    }

    // Update the current selection
    selected = application;
    active_window = window;
}

void ApplicationSelector::advise_focus_lost(const miral::WindowInfo &window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        return;
    }

    auto application = window.application();
    if (!application)
    {
        return;
    }

    if (selected == application)
    {
        selected = nullptr;
    }
}

void ApplicationSelector::advise_delete_app(Application const& application)
{
    auto it = std::find(focus_list.begin(), focus_list.end(), application);
    if (it == focus_list.end())
    {
        mir::log_warning("ApplicationSelector::advise_delete_app could not delete the app.");
        return;
    }

    // If we delete the selected application, we will try to select the next available one.
    if (application == selected)
    {
        auto original_it = it;
        do {
            it++;
            if (it == focus_list.end())
                it = focus_list.begin();

            if (it == original_it)
            {
                break;
            }
        } while (!tools.can_select_application(*it, active_window));

        if (it != original_it)
            selected = *it;
    }

    focus_list.erase(it);
}

auto ApplicationSelector::next(bool reverse) -> Application
{
    if (focus_list.empty())
    {
        return nullptr;
    }

    if (!is_active())
    {
        originally_selected = focus_list.front();
        selected = originally_selected;
    }

    // Attempt to focus the next application after the selected application.
    auto it = std::find(focus_list.begin(), focus_list.end(), selected);
    auto start_it = it;

    Window next_window;
    do {
        if (reverse)
        {
            if (it == focus_list.begin())
            {
                it = focus_list.end() - 1;
            }
            else
            {
                it--;
            }
        }
        else
        {
            it++;
            if (it == focus_list.end())
            {
                it = focus_list.begin();
            }
        }

        // We made it all the way through the list but failed to find anything.
        if (it == start_it)
            break;
    } while (!tools.can_select_application(*it, next_window));

    // Swap the tree order first and then select the new window
    if (*it == originally_selected)
    {
        // Edge case: if we have gone full circle around the list back to the original app
        // then we will wind up in a situation where the original app - now in the second z-order
        // position - will be swapped with the final app, putting the final app in the second position.
        for (auto window: tools.info_for(selected).windows())
            tools.send_tree_to_back(window);
    }
    else
    {
        tools.swap_tree_order(next_window, active_window);
    }

    // next_window will be a garbage window in this case, so let's not select it
    if (it != start_it)
    {
        tools.select_active_window(next_window);
    }

    return *it;
}

auto ApplicationSelector::complete() -> Application
{
    if (!is_active())
    {
        return nullptr;
    }

    // Place the newly selected item at the front of the list.
    auto it = std::find(focus_list.begin(), focus_list.end(), selected);
    if (it != focus_list.end())
    {
        std::rotate(focus_list.begin(), it, it + 1);
    }

    originally_selected = nullptr;
    return selected;
}

void ApplicationSelector::cancel()
{
    Window window_to_select;
    if (tools.can_select_application(originally_selected, window_to_select))
    {
        tools.select_active_window(window_to_select);
    }
    else
    {
        mir::log_warning("ApplicationSelector::cancel: Failed to select the root.");
    }

    originally_selected = nullptr;
}

auto ApplicationSelector::is_active() -> bool
{
    return originally_selected != nullptr;
}

auto ApplicationSelector::get_focused() -> Application
{
    return selected;
}