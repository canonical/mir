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

#ifndef MIROIL_DISPLAY_LISTENER_WRAPPER_H
#define MIROIL_DISPLAY_LISTENER_WRAPPER_H

#include <mir/geometry/forward.h>

#include <memory>

namespace mir {
    namespace compositor { class DisplayListener; }
}

namespace miroil
{

class DisplayListenerWrapper
{
public:
    DisplayListenerWrapper(std::shared_ptr<mir::compositor::DisplayListener> const& display_listener);
    ~DisplayListenerWrapper();

    void add_display(mir::geometry::Rectangle const& area);
    void remove_display(mir::geometry::Rectangle const& area);

private:
    std::shared_ptr<mir::compositor::DisplayListener> const& display_listener;
};

}

#endif /* MIROIL_DISPLAY_LISTENER_WRAPPER_H */
