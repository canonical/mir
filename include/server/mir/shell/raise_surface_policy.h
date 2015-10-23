/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_RAISE_SURFACE_POLICY_H_
#define MIR_RAISE_SURFACE_POLICY_H_

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{

/**
 * Policy to determine whether a client raise request should be honoured.
 *
 * Client requests to raise and focus surfaces (also known as presenting a surface)
 * are first checked against this policy. If the policy denies a request no further
 * action is taken.
 *
 * This is where a policy for focus stealing prevention should be implemented.
 */
class RaiseSurfacePolicy
{
public:
    RaiseSurfacePolicy() = default;
    virtual ~RaiseSurfacePolicy() = default;

    RaiseSurfacePolicy(RaiseSurfacePolicy const&) = delete;
    RaiseSurfacePolicy& operator=(RaiseSurfacePolicy const&) = delete;

    /**
     *  Takes a surface that is requesting to be raised and a timestamp then
     *  determines if that surface should be raised.
     *
     *  \param [in] surface_candidate  Surface requesting to be raised
     *  \param [in] timestamp          Timestamp from the event that caused the raise request
     *  return                         True if the surface should be raised, otherwise false
     */
    virtual bool should_raise_surface(
        std::shared_ptr<scene::Surface> const& surface_candidate,
        uint64_t timestamp) const = 0;
};

}
}

#endif // MIR_RAISE_SURFACE_POLICY_H_
