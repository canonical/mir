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
#include <mir/scene/session.h>
#include <mir/log.h>

using namespace miral;

ApplicationSelector::ApplicationSelector(const miral::WindowManagerTools& in_tools)
    : tools{in_tools}
{}

auto ApplicationSelector::start(bool in_reverse) -> Application
{
    if (is_active())
    {
        mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT,
                 "Cannot call ApplicationSelector::start when the ApplicationSelector is active.");
        return nullptr;
    }

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

    selected = tools.info_for(application);
    reverse = in_reverse;
    is_started = true;
    raise_next();
    return selected.application();
}

auto ApplicationSelector::next() -> Application
{
    if (!is_active())
    {
        mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT,
                 "Cannot call ApplicationSelector::next when the ApplicationSelector is not active. Call ApplicationSelector::start first.");
        return nullptr;
    }

    raise_next();
    return selected.application();
}

auto ApplicationSelector::complete() -> Application
{
    if (!is_active())
    {
        mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT,
                 "Cannot call ApplicationSelector::stop when the ApplicationSelector is not active. Call ApplicationSelector::start first.");
        return nullptr;
    }

    is_started = false;
    if (tools.can_focus_application(selected.application())) {
        // Note: The null checks here are mostly to accommodate the tests, however they are a good idea.
        auto application = selected.application();
        auto surface = application ? application->default_surface() : nullptr;
        if (surface)
        {
            auto window = tools.info_for(surface).window();
            tools.select_active_window(window);
        }
        else
        {
            mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT,
                     "ApplicationSelector::complete: Failed to select the active window.");
        }
    }

    return selected.application();
}

auto ApplicationSelector::is_active() -> bool
{
    return is_started;
}

void ApplicationSelector::raise_next()
{
    // First, find a suitable application in the list based off of the direction.
    // If we encounter the presently selected application again, that means
    // we have gone all the way around the list, so we can "break".
    auto selected_app = selected.application();
    auto app_info = selected;
    do {
        app_info = reverse ? tools.get_previous_application_info(app_info)
            : tools.get_next_application_info(app_info);
        if (app_info.application() == selected_app) break;
    } while (!tools.can_focus_application(app_info.application()));

    // Next, raise the window, but note that we do not actually focus it.
    // Note: The null checks here are mostly to accommodate the tests, however they are a good idea.
    auto application = app_info.application();
    auto surface = application ? application->default_surface() : nullptr;
    if (surface)
    {
        auto window = tools.info_for(surface).window();
        tools.raise_tree(window);
    }
    else
    {
        mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT,
                 "ApplicationSelector::raise_next: Failed to raise the newly selected window.");
    }
    selected = app_info;
}