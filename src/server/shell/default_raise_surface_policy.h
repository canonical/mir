/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_DEFAULT_RAISE_SURFACE_POLICY_H_
#define MIR_DEFAULT_RAISE_SURFACE_POLICY_H_

#include "mir/shell/raise_surface_policy.h"
#include "mir/input/event_filter.h"

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{

/**
 * The default behaviour is to check each MirEvent for input events that are key or
 * pointer/touch up/down. Save that timestamp, this being the last input event timestamp
 * that we have received. From there compare that last received input event timestamp to
 * the timestamp that we given by the client from an event. If we are lower then the last
 * input event we dont want to steal focus!
 */
class DefaultRaiseSurfacePolicy : public RaiseSurfacePolicy, public virtual input::EventFilter
{
public:
    DefaultRaiseSurfacePolicy() = default;

    bool should_raise_surface(
        std::shared_ptr<scene::Surface> const& surface_candidate,
        uint64_t timestamp) const override;

    bool handle(MirEvent const& event) override;

private:
    uint64_t last_input_event_timestamp{0};

};

}
}

#endif // MIR_DEFAULT_RAISE_SURFACE_POLICY_H_
