/*
 * Copyright © 2014 Canonical Ltd.
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
 */

#ifndef MIR_COMPOSITOR_FRAME_DROPPING_POLICY_H_
#define MIR_COMPOSITOR_FRAME_DROPPING_POLICY_H_

#include <functional>

namespace mir
{
namespace compositor
{
/**
 * \brief Policy to determine when to drop a frame from a client
 *
 * The FrameDroppingPolicy objects are constructed from a
 * \ref FrameDroppingPolicyFactory
 *
 * The frame dropping mechanism is provided as the
 * \a drop_frames argument of \ref FrameDroppingPolicyFactory::create_policy
 *
 * The policy may decide to drop a frame any time that there is an outstanding
 * swap - namely, when there have been more calls to \ref swap_now_blocking
 * than to \ref swap_unblocked
 */
class FrameDroppingPolicy
{
public:
    virtual ~FrameDroppingPolicy() = default;

    FrameDroppingPolicy(FrameDroppingPolicy const&) = delete;
    FrameDroppingPolicy& operator=(FrameDroppingPolicy const&) = delete;

    /**
     * \brief Notify that a swap is now blocking
     */
    virtual void swap_now_blocking() = 0;
    /**
     * \brief Notify that previous swap is no longer blocking
     */
    virtual void swap_unblocked() = 0;

protected:
    /**
     * \note FrameDroppingPolicies should not be constructed directly;
     *       use a \ref FrameDroppingPolicyFactory
     */
    FrameDroppingPolicy() = default;
};

}
}

#endif // MIR_COMPOSITOR_FRAME_DROPPING_POLICY_H_
