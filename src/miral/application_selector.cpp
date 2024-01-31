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

namespace miral
{

template <typename T>
class WindowCyclingBehavior
{
public:
    WindowCyclingBehavior(
        std::function<bool(T const&, T const&)> comparison_func,
        std::function<bool(T const&)> can_select,
        std::function<void(T const&, T const&, T const&)> on_raised,
        std::function<void(T const&)> on_cancelled,
        std::function<void(T&)> reset
    ) : comparison_func{comparison_func},
        can_select{can_select},
        on_raised{on_raised},
        on_cancelled{on_cancelled},
        reset{reset}
    {
    }

    WindowCyclingBehavior(
        std::function<bool(T const&, T const&)> comparison_func,
        std::function<bool(T const&)> can_select,
        std::function<void(T const& previously_selected, T const& newly_selected, T const& originally_selected)> on_raised,
        std::function<void(T const&)> on_cancelled,
        std::function<void(T&)> reset,
        std::vector<T> focus_list,
        T originally_selected,
        T selected,
        bool is_active
    ) : comparison_func{comparison_func},
        can_select{can_select},
        on_raised{on_raised},
        on_cancelled{on_cancelled},
        reset{reset},
        focus_list{focus_list},
        originally_selected{originally_selected},
        selected{selected},
        is_active{is_active}
    {
    }

    WindowCyclingBehavior(WindowCyclingBehavior const&) = delete;
    auto operator=(WindowCyclingBehavior const&) -> WindowCyclingBehavior& = delete;

    void add(T const& t)
    {
        focus_list.push_back(t);
    }

    void remove(T const& t)
    {
        auto it = find(t);
        if (it == focus_list.end())
        {
            mir::log_warning("WindowCyclingBehavior::remove could not find item to remove");
            return;
        }

        // If we delete the selected application, we will try to select the next available one.
        if (comparison_func(*it, selected))
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
            } while (!can_select(*it));

            if (it != original_it)
                selected = *it;
        }

        focus_list.erase(it);
    }

    void focus(T const& t)
    {
        auto it = find(t);
        if (it == focus_list.end())
        {
            mir::log_error("WindowCyclingBehavior::focus: attempting to focus an item that is not in the focus list");
            return;
        }

        if (!is_active)
        {
            // If we are not active, we move the newly focused item to the front of the list.
            if (it != focus_list.end())
                std::rotate(focus_list.begin(), it, it + 1);
        }

        selected = t;
    }

    void unfocus(T const& t)
    {
        if (comparison_func(t, selected))
        {
            reset(selected);
        }
    }

    auto next() -> T
    {
        return advance(false);
    }

    auto prev() -> T
    {
        return advance(true);
    }

    auto complete() -> T
    {
        if (!is_active)
        {
            mir::log_warning("WindowCyclingBehavior::complete: Cannot complete while not active");
            return T();
        }

        // Place the newly selected item at the front of the list.
        auto it = find(selected);
        if (it != focus_list.end())
        {
            std::rotate(focus_list.begin(), it, it + 1);
        }

        reset(originally_selected);
        is_active = false;
        return *it;
    }

    auto cancel() -> T
    {
        if (!is_active)
        {
            mir::log_warning("WindowCyclingBehavior::complete: Cannot cancel while not active");
            return T();
        }

        on_cancelled(originally_selected);
        reset(originally_selected);
        is_active = false;
        return originally_selected;
    }

    auto get_focused() -> T
    {
        return selected;
    }

    auto get_originally_focused() -> T
    {
        return originally_selected;
    }

    bool get_is_active()
    {
        return is_active;
    }

    std::vector<T> const& get_focus_list() { return focus_list; }

private:
    auto find(T const& application) -> std::vector<T>::iterator
    {
        return std::find_if(focus_list.begin(), focus_list.end(), [&](T const& existing_application)
        {
            return comparison_func(application, existing_application);
        });
    }

    auto advance(bool reverse) -> T
    {
        if (focus_list.empty())
            return T();

        if (!is_active)
        {
            is_active = true;
            originally_selected = focus_list.front();
            selected = originally_selected;
        }

        // Attempt to focus the next element after the selected element.
        auto it = find(selected);
        auto start_it = it;

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
        } while (!can_select(*it));

        on_raised(*start_it, *it, originally_selected);
        return *it;
    }

    std::function<bool(T const&, T const&)> comparison_func;
    std::function<bool(T const&)> can_select;
    std::function<void(T const&, T const&, T const&)> on_raised;
    std::function<void(T const&)> on_cancelled;
    std::function<void(T&)> reset;

    std::vector<T> focus_list;
    /// The application that was selected when next was first called
    T originally_selected;

    /// The application that is currently selected.
    T selected;

    bool is_active = false;
};

}

ApplicationSelector::ApplicationSelector(miral::WindowManagerTools const& _tools) :
    tools{_tools},
    window_cycler{std::make_unique<WindowCyclingBehavior<WeakApplication>>(
        get_comparison_func(),
        get_can_select_func(tools),
        get_on_raised_func(tools),
        get_on_cancelled_func(tools),
        get_reset_func()
    )}
{
}

ApplicationSelector::~ApplicationSelector() = default;

ApplicationSelector::ApplicationSelector(miral::ApplicationSelector const& other) :
    tools{other.tools}
{
    window_cycler = std::make_unique<WindowCyclingBehavior<WeakApplication>>(
        get_comparison_func(),
        get_can_select_func(tools),
        get_on_raised_func(tools),
        get_on_cancelled_func(tools),
        get_reset_func(),
        other.window_cycler->get_focus_list(),
        other.window_cycler->get_originally_focused(),
        other.window_cycler->get_focused(),
        other.window_cycler->get_is_active()
    );
}

auto ApplicationSelector::operator=(ApplicationSelector const& other) -> ApplicationSelector&
{
    tools = other.tools;
    window_cycler = std::make_unique<WindowCyclingBehavior<WeakApplication>>(
        get_comparison_func(),
        get_can_select_func(tools),
        get_on_raised_func(tools),
        get_on_cancelled_func(tools),
        get_reset_func(),
        other.window_cycler->get_focus_list(),
        other.window_cycler->get_originally_focused(),
        other.window_cycler->get_focused(),
        other.window_cycler->get_is_active()
    );
    return *this;
}

void ApplicationSelector::advise_new_app(Application const& application)
{
    window_cycler->add(application);
}

void ApplicationSelector::advise_focus_gained(WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::log_error("ApplicationSelector::advise_focus_gained: Could not find the window");
        return;
    }

    auto application = window.application();
    if (!application)
    {
        mir::log_error("ApplicationSelector::advise_focus_gained: Could not find the application");
        return;
    }

    window_cycler->focus(application);
}

void ApplicationSelector::advise_focus_lost(miral::WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::log_error("ApplicationSelector::advise_focus_lost: Could not find the window");
        return;
    }

    auto application = window.application();
    if (!application)
    {
        mir::log_error("ApplicationSelector::advise_focus_lost: Could not find the application");
        return;
    }

    window_cycler->unfocus(application);
}

void ApplicationSelector::advise_delete_app(Application const& application)
{
    window_cycler->remove(application);
}

auto ApplicationSelector::next() -> Application
{
    return window_cycler->next().lock();
}

auto ApplicationSelector::prev() -> Application
{
    return window_cycler->prev().lock();
}

auto ApplicationSelector::complete() -> Application
{
    return window_cycler->complete().lock();
}

void ApplicationSelector::cancel()
{
    window_cycler->cancel();
}

auto ApplicationSelector::is_active() -> bool
{
    return window_cycler->get_is_active();
}

auto ApplicationSelector::get_focused() -> Application
{
    return window_cycler->get_focused().lock();
}

auto ApplicationSelector::get_comparison_func() -> std::function<bool(ApplicationSelector::WeakApplication const&, ApplicationSelector::WeakApplication const&)>
{
    return [](ApplicationSelector::WeakApplication const& first, ApplicationSelector::WeakApplication const& second)
    {
        return !first.owner_before(second) && !second.owner_before(first);;
    };
}

auto ApplicationSelector::get_can_select_func(WindowManagerTools& tools) -> std::function<bool(ApplicationSelector::WeakApplication const&)>
{
    return [&](ApplicationSelector::WeakApplication const& session)
    {
        return tools.window_to_select_application(session.lock()) != std::nullopt;
    };
}

auto ApplicationSelector::get_on_raised_func(WindowManagerTools& tools)
    -> std::function<void(ApplicationSelector::WeakApplication const&, ApplicationSelector::WeakApplication const&, ApplicationSelector::WeakApplication const&)>
{
    return [&](ApplicationSelector::WeakApplication const& previously_selected,
               ApplicationSelector::WeakApplication const& newly_selected,
               ApplicationSelector::WeakApplication const& originally_selected)
    {
        auto previous_session = previously_selected.lock();
        auto new_session = newly_selected.lock();
        auto original_session = originally_selected.lock();

        auto previous_window = tools.active_window();
        auto new_window = tools.window_to_select_application(new_session);
        if (previous_session == new_session || new_window == std::nullopt)
            return;

        if (new_session == original_session)
        {
            // Edge case: if we have gone full circle around the list back to the original app
            // then we will wind up in a situation where the original app - now in the second z-order
            // position - will be swapped with the final app, putting the final app in the second position.
            for (auto const& window: tools.info_for(new_session).windows())
                tools.send_tree_to_back(window);
        }
        else
        {
            tools.swap_tree_order(new_window.value(), previous_window);
        }

        tools.select_active_window(new_window.value());
    };
}

auto ApplicationSelector::get_on_cancelled_func(WindowManagerTools& tools) -> std::function<void(ApplicationSelector::WeakApplication const&)>
{
    return [&](ApplicationSelector::WeakApplication const& session)
    {
        auto window_to_select = tools.window_to_select_application(session.lock());
        if (window_to_select != std::nullopt)
        {
            tools.select_active_window(window_to_select.value());
        }
    };
}

auto ApplicationSelector::get_reset_func() -> std::function<void(ApplicationSelector::WeakApplication&)>
{
    return [](ApplicationSelector::WeakApplication& session)
    {
        session.reset();
    };
}