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

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{
class FocusController;

/**
 * The default raise policy for a surface raise request
 */
class DefaultRaiseSurfacePolicy : public RaiseSurfacePolicy
{
public:
    /**
     * DefaultRaiseSurfacePolicy uses the focus controller to get access to the
     * currently focused window.
     *
     * \param [in] focus_controller  The focus controller for the shell
     */
    DefaultRaiseSurfacePolicy(std::shared_ptr<FocusController> const& focus_controller);

    /**
     * The default behaviour is to use the focus cotroller to get the currently focused
     * window. From there compare if the timestamp is less then the currently focused
     * windows last input event timestamp. This means motion events will the raise policy
     * to fail as it will update the last input event timestamp.
     *
     *  \param [in] surface_candidate  Surface requesting to be raised
     *  \param [in] timestamp          Timestamp from the event that caused the raise request
     *  return                         True if the surface should be raised, otherwise false
     */
    bool should_raise_surface(
        std::shared_ptr<scene::Surface> const& surface_candidate,
        uint64_t timestamp) const override;

private:
    std::shared_ptr<FocusController> const focus_controller;
};

}
}

#endif // MIR_DEFAULT_RAISE_SURFACE_POLICY_H_
