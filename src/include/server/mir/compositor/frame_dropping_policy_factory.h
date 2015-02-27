/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_COMPOSITOR_FRAME_DROPPING_POLICY_FACTORY_H_
#define MIR_COMPOSITOR_FRAME_DROPPING_POLICY_FACTORY_H_

#include <memory>

namespace mir
{
class LockableCallback;

namespace compositor
{
class FrameDroppingPolicy;

/**
 * \brief Creator of FrameDroppingPolicies
 *
 * The FrameDroppingPolicyFactory is how you go from a means of dropping frames -
 * the \a drop_frames parameter of \ref create_policy -
 * to a \ref FrameDroppingPolicy
 */
class FrameDroppingPolicyFactory
{
public:
    FrameDroppingPolicyFactory() = default;
    virtual ~FrameDroppingPolicyFactory() = default;

    FrameDroppingPolicyFactory(FrameDroppingPolicyFactory const&) = delete;
    FrameDroppingPolicyFactory& operator=(FrameDroppingPolicyFactory const&) = delete;

    /**
     * \brief Create a FrameDroppingPolicy that will call \a drop_frame when it
     *       decides to drop a frame
     *
     * A LockableCallback allows the user to preserve lock ordering
     * in situations where FrameDroppingPolicy methods need to be called under
     * external lock and the callback implementation needs to run code protected
     * by the same lock. A FrameDroppingPolicy implementation may have internal
     * locks of its own, which maybe acquired during callback dispatching;
     * to preserve lock ordering LockableCallback::lock will be invoked during
     * callback dispatch before any internal locks are acquired.
     *
     * \param drop_frame Function to call when a frame needs to be dropped
     * \param lock       Function called within the callback dispatcher context
     *                   before any internal locks are acquired.
     * \param unlock     Function called within the callback dispatcher context
     *                   after any internal locks are released.
     */
    virtual std::unique_ptr<FrameDroppingPolicy> create_policy(
        std::shared_ptr<LockableCallback> const& drop_frame) const = 0;
};

}
}

#endif // MIR_COMPOSITOR_FRAME_DROPPING_POLICY_FACTORY_H_
