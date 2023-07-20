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
}

void ApplicationSelector::advise_delete_app(Application const& application)
{
    auto it = std::find(focus_list.begin(), focus_list.end(), application);
    if (it != focus_list.end())
    {
        focus_list.erase(it);
    }
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

    // Attempt to focus the next application after the originally selected application.
    auto it = std::find(focus_list.begin(), focus_list.end(), selected);
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
            if (it == focus_list.end() - 1)
            {
                it = focus_list.begin();
            }
            else
            {
                it++;
            }
        }
    } while (!tools.can_focus_application(*it));

    if (!try_select_application(*it))
    {
        mir::log_warning("ApplicationSelector::next: Failed to select the next application.");
    }

    selected = *it;
    return selected;
}

auto ApplicationSelector::complete() -> Application
{
    if (!is_active())
    {
        mir::log_warning("Cannot call ApplicationSelector::stop when the ApplicationSelector is not active. Call ApplicationSelector::start first.");
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
    if (!try_select_application(originally_selected))
    {
        mir::log_warning("ApplicationSelector::cancel: Failed to select the root.");
    }

    originally_selected = nullptr;
}

auto ApplicationSelector::is_active() -> bool
{
    return originally_selected != nullptr;
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