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
    originally_selected_it{other.originally_selected_it},
    selected{other.selected}
{
}

auto ApplicationSelector::operator=(ApplicationSelector const& other) -> ApplicationSelector&
{
    tools = other.tools;
    focus_list = other.focus_list;
    originally_selected_it = other.originally_selected_it;
    selected = other.selected;
    return *this;
}

void ApplicationSelector::advise_new_window(WindowInfo const& window_info)
{
    focus_list.push_back(window_info.window());
}

void ApplicationSelector::select(miral::Window const& window)
{
    if (selected)
    {
        // Restore the previously selected window to its previous state when another is selected
        auto const& info = tools.info_for(selected);
        if (restore_state && restore_state != info.state())
        {
            WindowSpecification spec;
            spec.state() = restore_state.value();
            tools.modify_window(selected, spec);
            restore_state.reset();
        }
    }

    // Update the current selection
    selected = window;
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
            std::rotate(focus_list.begin(), it, it + 1);
        else
            mir::fatal_error("advise_focus_gained: window was not found in the list");
    }

    select(window);
}

void ApplicationSelector::advise_focus_lost(miral::WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::fatal_error("advise_focus_lost: window_info did not reference a window");
        return;
    }

    select(Window());
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
    auto original_it = it;
    if (*it == selected)
    {
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
    else
        selected = Window();

    focus_list.erase(original_it);
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
        return {};

    // Place the newly selected item at the front of the list.
    auto it = find(selected);
    if (it != focus_list.end())
    {
        std::rotate(focus_list.begin(), it, it + 1);
    }

    originally_selected_it = focus_list.end();
    is_active_ = false;
    restore_state.reset();
    return selected;
}

void ApplicationSelector::cancel()
{
    if (!is_active())
    {
        mir::log_warning("Cannot cancel while not active");
        return;
    }

    tools.select_active_window(*originally_selected_it);
    originally_selected_it = focus_list.end();
    is_active_ = false;
}

auto ApplicationSelector::is_active() const -> bool
{
    return is_active_;
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
        originally_selected_it = focus_list.begin();
        selected = focus_list.front();
        is_active_ = true;
    }

    // Attempt to focus the next application after the selected application.
    auto it = find(selected);

    std::optional<Window> next_window = std::nullopt;
    do {
        if (reverse)
        {
            if (it == focus_list.begin())
                it = focus_list.end() - 1;
            else
                it--;
        }
        else
        {
            it++;
            if (it == focus_list.end())
                it = focus_list.begin();
        }

        // We made it all the way through the list but failed to find anything.
        // This means that there is no other selectable window in the list but
        // the currently selected one, so we don't need to select anything.
        if (*it == selected)
        {
            if(!tools.info_for(selected).is_visible())
                tools.select_active_window(selected);
            return selected;
        }

        if (within_app)
        {
            if (it->application() == (*originally_selected_it).application() && tools.can_select_window(*it))
                next_window = *it;
            else
                next_window = std::nullopt;
        }
        else
        {
            // Check if we've already encountered this application. If so, we shouldn't try to select it again
            bool already_encountered = false;
            for (auto prev_it = originally_selected_it; prev_it != it; prev_it++)
            {
                if (prev_it == focus_list.end())
                    prev_it = focus_list.begin();

                if (prev_it->application() == it->application())
                {
                    next_window = std::nullopt;
                    already_encountered = true;
                    break;
                }
            }
            if (!already_encountered)
                next_window = tools.window_to_select_application(it->application());
        }
    } while (next_window == std::nullopt);

    if (next_window == std::nullopt)
        return selected;

    if (it == originally_selected_it)
    {
        // Edge case: if we have gone full circle around the list back to the original app
        // then we will wind up in a situation where the original app is in the 2nd position
        // in the list, while the last app is in the 1st position. Hence, we send the selected
        // app to the back of the list.
        auto application = selected.application();
        for (auto& window: tools.info_for(application).windows())
            tools.send_tree_to_back(window);
    }
    else
        tools.swap_tree_order(next_window.value(), selected);

    auto next_state_to_preserve =  tools.info_for(next_window.value()).state();
    tools.select_active_window(next_window.value());
    restore_state = next_state_to_preserve;
    return next_window.value();
}

auto ApplicationSelector::find(Window window) -> std::vector<Window>::iterator
{
    return std::find_if(focus_list.begin(), focus_list.end(), [&](Window const& other)
    {
        return window == other;
    });
}