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

#include <iostream>

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{

/**
 * The default behaviour is to use the focus cotroller to get the currently focused
 * window. From there compare if the input event timestamp is less then the currently
 * focused windows last input event timestamp.
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
