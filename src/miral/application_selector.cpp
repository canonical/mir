//
// Created by matthew on 7/18/23.
//

#include "application_selector.h"
#include <mir/scene/session.h>

using namespace miral;

ApplicationSelector::ApplicationSelector(const miral::WindowManagerTools & in_tools) :
    tools{in_tools}
{}

void ApplicationSelector::start(bool in_reverse)
{
    if (is_active())
    {
        return;
    }

    printf("Started\n");
    selected = tools.info_for(tools.active_window().application());
    reverse = in_reverse;
    is_started = true;
    raise_next();
}

auto ApplicationSelector::next() -> Application
{
    if (!is_active())
    {
        // TODO: Error
        return nullptr;
    }

    printf("Next\n");
    raise_next();
    return nullptr;
}

auto ApplicationSelector::complete() -> Application
{
    if (!is_active())
    {
        // TODO: Error
        return nullptr;
    }

    printf("Complete\n");
    is_started = false;
    tools.focus_this_application(selected.application());
    return selected.application();
}

auto ApplicationSelector::is_active() -> bool
{
    return is_started;
}

void ApplicationSelector::raise_next()
{
    auto app_info = reverse ? tools.get_previous_application_info(selected) : tools.get_next_application_info(selected);
    tools.raise_tree(tools.info_for(app_info.application()->default_surface()).window());
    selected = app_info;
}