/*
 * Copyright (C) Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "miroil/display_listener_wrapper.h"
#include "mir/compositor/display_listener.h"

namespace miroil
{

DisplayListenerWrapper::DisplayListenerWrapper(std::shared_ptr<mir::compositor::DisplayListener> const & display_listener)
 : display_listener(display_listener)
{
}

DisplayListenerWrapper::~DisplayListenerWrapper()
{
}

void DisplayListenerWrapper::add_display(mir::geometry::Rectangle const& area)
{
    display_listener->add_display(area);
}

void DisplayListenerWrapper::remove_display(mir::geometry::Rectangle const& area)
{
    display_listener->remove_display(area);
}

}
