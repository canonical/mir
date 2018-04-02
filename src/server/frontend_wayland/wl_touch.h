/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_TOUCH_H
#define MIR_FRONTEND_WL_TOUCH_H

#include "generated/wayland_wrapper.h"

// from "mir_toolkit/events/event.h"
struct MirTouchEvent;

namespace mir
{

class Executor;

namespace frontend
{

class WlTouch : public wayland::Touch
{
public:
    WlTouch(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlTouch*)> const& on_destroy);

    ~WlTouch();

    void handle_event(MirTouchEvent const* event, wl_resource* target);

private:
    std::function<void(WlTouch*)> on_destroy;

    void release() override;
};

}
}

#endif // MIR_FRONTEND_WL_TOUCH_H
