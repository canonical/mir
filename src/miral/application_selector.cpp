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

void ApplicationSelector::start(bool in_reverse)
{
    if (is_active())
    {
        return;
    }

    selected = tools.info_for(tools.active_window().application());
    reverse = in_reverse;
    is_started = true;
    raise_next();
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
    return nullptr;
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
        auto window = tools.info_for(selected.application()->default_surface()).window();
        tools.select_active_window(window);
    }
    return selected.application();
}

auto ApplicationSelector::is_active() -> bool
{
    return is_started;
}

void ApplicationSelector::raise_next()
{
    auto app_info = selected;
    do {
        app_info = reverse ? tools.get_previous_application_info(app_info) : tools.get_next_application_info(app_info);
    } while (!tools.can_focus_application(app_info.application()));
    tools.raise_tree(tools.info_for(app_info.application()->default_surface()).window());
    selected = app_info;
}