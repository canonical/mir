//
// Created by matthew on 7/18/23.
//

#include "application_selector.h"

using namespace miral;

ApplicationSelector::ApplicationSelector(const miral::WindowManagerTools & in_tools) :
    tools(in_tools)
{}

void ApplicationSelector::start()
{
    is_active = true;
}

auto ApplicationSelector::next() -> Application
{
    if (!is_active)
    {
        // TODO: Error
    }

    return nullptr;
}

auto ApplicationSelector::stop() -> Application
{
    is_active = false;
    return nullptr;
}