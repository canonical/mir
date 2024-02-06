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
#include <miral/application.h>
#include <mir/log.h>
#include <mir/scene/session.h>

using namespace miral;

ApplicationSelector::ApplicationSelector(miral::WindowManagerTools const& in_tools) :
    tools{in_tools}
{}

ApplicationSelector::~ApplicationSelector() = default;

ApplicationSelector::ApplicationSelector(miral::ApplicationSelector const& other) :
    tools{other.tools},
    focus_list{other.focus_list},
    originally_selected{other.originally_selected},
    selected{other.selected}
{
}

auto ApplicationSelector::operator=(ApplicationSelector const& other) -> ApplicationSelector&
{
    tools = other.tools;
    focus_list = other.focus_list;
    originally_selected = other.originally_selected;
    selected = other.selected;
    return *this;
}

void ApplicationSelector::advise_new_window(WindowInfo const& window_info)
{
    focus_list.push_back(window_info.window());
}

void ApplicationSelector::advise_focus_gained(WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::fatal_error("advise_focus_gained: window_info did not reference a window");
    }

    auto it = find(window);
    if (!is_active())
    {
        // If we are not active, we move the newly focused item to the front of the list.
        if (it != focus_list.end())
        {
            std::rotate(focus_list.begin(), it, it + 1);
        }
    }

    // Update the current selection
    selected = window;
}

void ApplicationSelector::advise_focus_lost(miral::WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::fatal_error("advise_focus_lost: window_info did not reference a window");
        return;
    }

    selected = Window();
}

void ApplicationSelector::advise_delete_window(WindowInfo const& window_info)
{
    auto it = find(window_info.window());
    if (it == focus_list.end())
    {
        mir::log_warning("ApplicationSelector::advise_delete_app could not delete the app.");
        return;
    }

    // If we delete the selected window, we will try to select the next available one
    if (*it == selected)
    {
        auto original_it = it;
        do {
            it++;
            if (it == focus_list.end())
                it = focus_list.begin();

            if (it == original_it)
                break;
        } while (!tools.can_select_window(*it));

        if (it != original_it)
            selected = *it;
        else
            selected = Window();
    }

    focus_list.erase(it);
}

auto ApplicationSelector::next(bool within_app) -> Window
{
    return advance(false, within_app);
}

auto ApplicationSelector::prev(bool within_app) -> Window
{
    return advance(true, within_app);
}

auto ApplicationSelector::complete() -> Window
{
    if (!is_active())
    {
        mir::log_warning("complete: application selector is not active");
        return {};
    }

    // Place the newly selected item at the front of the list.
    auto it = find(selected);
    if (it != focus_list.end())
    {
        std::rotate(focus_list.begin(), it, it + 1);
    }

    originally_selected = Window();
    is_moving = false;
    return selected;
}

void ApplicationSelector::cancel()
{
    tools.select_active_window(originally_selected);
    originally_selected = Window();
    is_moving = false;
}

auto ApplicationSelector::is_active() const -> bool
{
    return is_moving;
}

auto ApplicationSelector::get_focused() -> Window
{
    return selected;
}

auto ApplicationSelector::advance(bool reverse, bool within_app) -> Window
{
    if (focus_list.empty())
    {
        return {};
    }

    if (!is_active())
    {
        originally_selected = focus_list.front();
        selected = originally_selected;
        is_moving = true;
    }

    // Attempt to focus the next application after the selected application.
    auto it = find(selected);
    auto start_it = it;

    std::optional<Window> next_window = std::nullopt;
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

        if (within_app)
        {
            if (it->application() == originally_selected.application() && tools.can_select_window(*it))
                next_window = *it;
            else
                next_window = std::nullopt;
        }
        else
        {
            next_window = tools.window_to_select_application(it->application());
        }
    } while (next_window == std::nullopt);

    if (it == start_it || next_window == std::nullopt)
    {
        // next_window will be a garbage window in this case, so let's not select it
        return *start_it;
    }

    // Swap the tree order first and then select the new window
    if (*it == originally_selected)
    {
        // Edge case: if we have gone full circle around the list back to the original app
        // then we will wind up in a situation where the original app - now in the second z-order
        // position - will be swapped with the final app, putting the final app in the second position.
        auto application = it->application();
        for (auto& window: tools.info_for(application).windows())
            tools.send_tree_to_back(window);
    }
    else
        tools.swap_tree_order(next_window.value(), selected);

    tools.select_active_window(next_window.value());
    return *it;
}

auto ApplicationSelector::find(Window window) -> std::vector<Window>::iterator
{
    return std::find_if(focus_list.begin(), focus_list.end(), [&](Window const& other)
    {
        return window == other;
    });
}