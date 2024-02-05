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

namespace
{
struct WeakApplicationSelector
{
    using value_t = std::weak_ptr<mir::scene::Session>;

    explicit WeakApplicationSelector(WindowManagerTools const& tools)
        : tools{tools}
    {}

    static auto equals(value_t const& first, value_t const& second) -> bool
    {
        return !first.owner_before(second) && !second.owner_before(first);;
    }

    auto selectable(value_t const& session) const -> bool
    {
        return tools.window_to_select_application(session.lock()) != std::nullopt;
    }

    void
    on_selected(value_t const& previously_selected, value_t const& newly_selected, value_t const& originally_selected)
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
        } else
        {
            tools.swap_tree_order(new_window.value(), previous_window);
        }

        tools.select_active_window(new_window.value());
    }

    void on_cancelled(value_t const& session)
    {
        auto window_to_select = tools.window_to_select_application(session.lock());
        if (window_to_select != std::nullopt)
        {
            tools.select_active_window(window_to_select.value());
        }
    }

    static void reset(value_t& session)
    {
        session.reset();
    }

    WindowManagerTools tools;
};

template<class SelectorBase>
class TabOrderSelector : public SelectorBase
{
public:
    using T = typename SelectorBase::value_t;
    using SelectorBase::SelectorBase;

    TabOrderSelector(TabOrderSelector const &) = delete;

    auto operator=(TabOrderSelector const &) -> TabOrderSelector & = delete;

    void add(T const& t)
    {
        focus_list.push_back(t);
    }

    void remove(T const& t)
    {
        auto it = find(t);
        if (it == focus_list.end())
        {
            mir::log_warning("TabOrderSelector::remove could not find item to remove");
            return;
        }

        // If we delete the selected application, we will try to select the next available one.
        if (SelectorBase::equals(*it, selected))
        {
            auto original_it = it;
            do
            {
                it++;
                if (it == focus_list.end())
                    it = focus_list.begin();

                if (it == original_it)
                {
                    break;
                }
            } while (!SelectorBase::selectable(*it));

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
            mir::log_error("TabOrderSelector::focus: attempting to focus an item that is not in the focus list");
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
        if (SelectorBase::equals(t, selected))
        {
            SelectorBase::reset(selected);
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
            mir::log_warning("TabOrderSelector::complete: Cannot complete while not active");
            return T();
        }

        // Place the newly selected item at the front of the list.
        auto it = find(selected);
        if (it != focus_list.end())
        {
            std::rotate(focus_list.begin(), it, it + 1);
        }

        SelectorBase::reset(originally_selected);
        is_active = false;
        return *it;
    }

    auto cancel() -> T
    {
        if (!is_active)
        {
            mir::log_warning("TabOrderSelector::complete: Cannot cancel while not active");
            return T();
        }

        SelectorBase::on_cancelled(originally_selected);
        SelectorBase::reset(originally_selected);
        is_active = false;
        return originally_selected;
    }

    auto get_focused() -> T
    {
        return selected;
    }

    bool get_is_active()
    {
        return is_active;
    }

private:
    auto find(T const& application) -> std::vector<T>::iterator
    {
        return std::find_if(focus_list.begin(), focus_list.end(), [&](T const &existing_application)
        {
            return SelectorBase::equals(application, existing_application);
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

        do
        {
            if (reverse)
            {
                if (it == focus_list.begin())
                {
                    it = focus_list.end() - 1;
                } else
                {
                    it--;
                }
            } else
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
        } while (!SelectorBase::selectable(*it));

        SelectorBase::on_selected(*start_it, *it, originally_selected);
        return *it;
    }

    std::vector<T> focus_list;
    /// The application that was selected when next was first called
    T originally_selected;

    /// The application that is currently selected.
    T selected;

    bool is_active = false;
};
}

struct ApplicationSelector::Impl : TabOrderSelector<WeakApplicationSelector>
{
    using TabOrderSelector<WeakApplicationSelector>::TabOrderSelector;
};

ApplicationSelector::ApplicationSelector(miral::WindowManagerTools const& _tools) :
    self{std::make_unique<Impl>(_tools)}
{
}

ApplicationSelector::~ApplicationSelector() = default;

void ApplicationSelector::advise_new_app(Application const& application)
{
    self->add(application);
}

void ApplicationSelector::advise_focus_gained(WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::fatal_error("ApplicationSelector::advise_focus_gained: Could not find the window");
        return;
    }

    auto application = window.application();
    if (!application)
    {
        mir::fatal_error("ApplicationSelector::advise_focus_gained: Could not find the application");
        return;
    }

    self->focus(application);
}

void ApplicationSelector::advise_focus_lost(miral::WindowInfo const& window_info)
{
    auto window = window_info.window();
    if (!window)
    {
        mir::fatal_error("ApplicationSelector::advise_focus_lost: Could not find the window");
        return;
    }

    auto application = window.application();
    if (!application)
    {
        mir::fatal_error("ApplicationSelector::advise_focus_lost: Could not find the application");
        return;
    }

    self->unfocus(application);
}

void ApplicationSelector::advise_delete_app(Application const& application)
{
    self->remove(application);
}

auto ApplicationSelector::next() -> Application
{
    return self->next().lock();
}

auto ApplicationSelector::prev() -> Application
{
    return self->prev().lock();
}

auto ApplicationSelector::complete() -> Application
{
    return self->complete().lock();
}

void ApplicationSelector::cancel()
{
    self->cancel();
}

auto ApplicationSelector::is_active() -> bool
{
    return self->get_is_active();
}

auto ApplicationSelector::get_focused() -> Application
{
    return self->get_focused().lock();
}
