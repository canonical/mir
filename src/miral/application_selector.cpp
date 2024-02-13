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

using namespace miral;

ApplicationSelector::ApplicationSelector(miral::WindowManagerTools const& in_tools) :
    tools{in_tools}
{}

ApplicationSelector::~ApplicationSelector() = default;

ApplicationSelector::ApplicationSelector(miral::ApplicationSelector const& other) :
    tools{other.tools},
    focus_list{other.focus_list},
    originally_selected{other.originally_selected},
    selected{other.selected},
    active_window{other.active_window}
{
}

auto ApplicationSelector::operator=(ApplicationSelector const& other) -> ApplicationSelector&
{
    tools = other.tools;
    focus_list = other.focus_list;
    originally_selected = other.originally_selected;
    selected = other.selected;
    active_window = other.active_window;
    return *this;
}

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

    auto it = find(application);
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

void ApplicationSelector::advise_focus_lost(miral::WindowInfo const& window_info)
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

    if (selected.lock() == application)
    {
        selected.reset();
    }
}

void ApplicationSelector::advise_delete_app(Application const& application)
{
    auto it = find(application);
    if (it == focus_list.end())
    {
        mir::log_warning("ApplicationSelector::advise_delete_app could not delete the app.");
        return;
    }

    // If we delete the selected application, we will try to select the next available one.
    if (application == selected.lock())
    {
        std::optional<Window> next_window = std::nullopt;
        auto original_it = it;
        do {
            it++;
            if (it == focus_list.end())
                it = focus_list.begin();

            if (it == original_it)
            {
                break;
            }
        } while ((next_window = tools.window_to_select_application(it->lock())) == std::nullopt);

        if (next_window != std::nullopt)
            active_window = next_window.value();

        if (it != original_it)
            selected = *it;
    }

    focus_list.erase(it);
}

auto ApplicationSelector::next() -> Application
{
    return advance(false);
}

auto ApplicationSelector::prev() -> Application
{
    return advance(true);
}

auto ApplicationSelector::complete() -> Application
{
    if (!is_active())
    {
        return nullptr;
    }

    // Place the newly selected item at the front of the list.
    auto it = find(selected);
    if (it != focus_list.end())
    {
        std::rotate(focus_list.begin(), it, it + 1);
    }

    originally_selected.reset();
    return selected.lock();
}

void ApplicationSelector::cancel()
{
    std::optional<Window> window_to_select;
    if ((window_to_select = tools.window_to_select_application(originally_selected.lock())) != std::nullopt)
    {
        tools.select_active_window(window_to_select.value());
    }
    else
    {
        mir::log_warning("ApplicationSelector::cancel: Failed to select the root.");
    }

    originally_selected.reset();
}

auto ApplicationSelector::is_active() -> bool
{
    return !originally_selected.expired();
}

auto ApplicationSelector::get_focused() -> Application
{
    return selected.lock();
}

auto ApplicationSelector::advance(bool reverse) -> Application
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
    } while ((next_window = tools.window_to_select_application(it->lock())) == std::nullopt);

    if (it == start_it || next_window == std::nullopt)
    {
        // next_window will be a garbage window in this case, so let's not select it
        return start_it->lock();
    }

    // Swap the tree order first and then select the new window
    if (it->lock() == originally_selected.lock())
    {
        // Edge case: if we have gone full circle around the list back to the original app
        // then we will wind up in a situation where the original app - now in the second z-order
        // position - will be swapped with the final app, putting the final app in the second position.
        for (auto window: tools.info_for(selected).windows())
            tools.send_tree_to_back(window);
    }
    else
        tools.swap_tree_order(next_window.value(), active_window);

    tools.select_active_window(next_window.value());
    return it->lock();
}

auto ApplicationSelector::find(WeakApplication application) -> std::vector<WeakApplication>::iterator
{
    return std::find_if(focus_list.begin(), focus_list.end(), [&](WeakApplication const& existing_application)
    {
        return !existing_application.owner_before(application) && !application.owner_before(existing_application);
    });
}