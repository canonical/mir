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

#include "wayland_wrapper.h"

#include "mir/geometry/point.h"

#include <unordered_map>
#include <functional>
#include <chrono>

namespace mir
{
class Executor;

namespace frontend
{
class WlSurface;

class WlTouch : public wayland::Touch
{
public:
    WlTouch(
        wl_resource* new_resource,
        std::function<void(WlTouch*)> const& on_destroy);

    ~WlTouch();

    void down(
        std::chrono::milliseconds const& ms,
        int32_t touch_id,
        WlSurface* root_surface,
        std::pair<float, float> const& root_position);
    void motion(
        std::chrono::milliseconds const& ms,
        int32_t touch_id,
        WlSurface* root_surface,
        std::pair<float, float> const& root_position);
    void up(std::chrono::milliseconds const& ms, int32_t touch_id);
    void frame();

private:
    std::function<void(WlTouch*)> on_destroy;
    /// Maps touch IDs to the surfaces the touch is on
    std::unordered_map<int32_t, wayland::Weak<WlSurface>> touch_id_to_surface;
    bool can_send_frame{false};

    void release() override;

    /// Maps every touch_id for every WlTouch to a stable and globally unique pointer
    /// Used for surface destory listener keys
    /// The returned pointer should only be used as a key, it will not necessarily point to anything meaningful/valid
    void const* unique_key_for(int32_t touch_id) const;
};

}
}

#endif // MIR_FRONTEND_WL_TOUCH_H
