/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the Implementationied warranty of
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
using WeakApplication = std::weak_ptr<mir::scene::Session>;

struct ApplicationSelector::Implementation
{
    Implementation(WindowManagerTools const& tools)
        : tools{tools}
    {
    }

    WindowManagerTools tools;

    /// Represents the current order of focus by application. Most recently focused
    /// applications are at the beginning, while least recently focused are at the end.
    std::vector<WeakApplication> focus_list;

    /// The application that was selected when next was first called
    WeakApplication originally_selected;

    /// The application that is currently selected.
    WeakApplication selected;
    Window active_window;

    auto advance(bool reverse) -> Application;
    auto is_active() -> bool;
};

ApplicationSelector::ApplicationSelector(miral::WindowManagerTools const& in_tools)
    : self{std::make_unique<Implementation>(in_tools)}
{}

ApplicationSelector::~ApplicationSelector() = default;

ApplicationSelector::ApplicationSelector(miral::ApplicationSelector const& other) :
    self{std::make_unique<Implementation>(other.self->tools)}
{
}

auto ApplicationSelector::operator=(ApplicationSelector const& other) -> ApplicationSelector&
{
    *self = *other.self;
    return *this;
}

void ApplicationSelector::advise_new_app(Application const& application)
{
    self->focus_list.push_back(application);
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

    auto it = std::find_if(self->focus_list.begin(), self->focus_list.end(), [application](WeakApplication const& existing_application)
    {
        return existing_application.lock() == application;
    });
    if (!self->is_active())
    {
        // If we are not active, we move the newly focused item to the front of the list.
        if (it != self->focus_list.end())
        {
            std::rotate(self->focus_list.begin(), it, it + 1);
        }
    }

    // Update the current selection
    self->selected = application;
    self->active_window = window;
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

    if (self->selected.lock() == application)
    {
        self->selected.reset();
    }
}

void ApplicationSelector::advise_delete_app(Application const& application)
{
    auto it = std::find_if(self->focus_list.begin(), self->focus_list.end(), [application](WeakApplication const& existing_application)
    {
        return existing_application.lock() == application;
    });
    if (it == self->focus_list.end())
    {
        mir::log_warning("ApplicationSelector::advise_delete_app could not delete the app.");
        return;
    }

    // If we delete the selected application, we will try to select the next available one.
    if (application == self->selected.lock())
    {
        std::optional<Window> next_window = std::nullopt;
        auto original_it = it;
        do {
            it++;
            if (it == self->focus_list.end())
                it = self->focus_list.begin();

            if (it == original_it)
            {
                break;
            }
        } while ((next_window = self->tools.window_to_select_application(it->lock())) == std::nullopt);

        if (next_window != std::nullopt)
            self->active_window = next_window.value();

        if (it != original_it)
            self->selected = *it;
    }

    self->focus_list.erase(it);
}

auto ApplicationSelector::next() -> Application
{
    return self->advance(false);
}

auto ApplicationSelector::prev() -> Application
{
    return self->advance(true);
}

auto ApplicationSelector::complete() -> Application
{
    if (!self->is_active())
    {
        return nullptr;
    }

    // Place the newly selected item at the front of the list.
    auto it = std::find_if(self->focus_list.begin(), self->focus_list.end(), [&](WeakApplication const& existing_application)
    {
        return existing_application.lock() == self->selected.lock();
    });
    if (it != self->focus_list.end())
    {
        std::rotate(self->focus_list.begin(), it, it + 1);
    }

    self->originally_selected.reset();
    return self->selected.lock();
}

void ApplicationSelector::cancel()
{
    std::optional<Window> window_to_select;
    if ((window_to_select = self->tools.window_to_select_application(self->originally_selected.lock())) != std::nullopt)
    {
        self->tools.select_active_window(window_to_select.value());
    }
    else
    {
        mir::log_warning("ApplicationSelector::cancel: Failed to select the root.");
    }

    self->originally_selected.reset();
}

auto ApplicationSelector::is_active() -> bool
{
    return self->is_active();
}

auto ApplicationSelector::get_focused() -> Application
{
    return self->selected.lock();
}

auto ApplicationSelector::Implementation::advance(bool reverse) -> Application
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
    auto it = std::find_if(focus_list.begin(), focus_list.end(), [&](WeakApplication const& existing_application)
    {
        return existing_application.lock() == selected.lock();
    });
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

    // Swap the tree order first and then select the new window
    if (it->lock() == originally_selected.lock())
    {
        // Edge case: if we have gone full circle around the list back to the original app
        // then we will wind up in a situation where the original app - now in the second z-order
        // position - will be swapped with the final app, putting the final app in the second position.
        for (auto window: tools.info_for(selected).windows())
            tools.send_tree_to_back(window);
    }
    else if (next_window != std::nullopt)
        tools.swap_tree_order(next_window.value(), active_window);

    // next_window will be a garbage window in this case, so let's not select it
    if (it != start_it && next_window != std::nullopt)
    {
        tools.select_active_window(next_window.value());
    }

    return it->lock();
}

auto ApplicationSelector::Implementation::is_active() -> bool
{
    return !originally_selected.expired();
}