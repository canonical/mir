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

#ifndef MIR_COMPOSITOR_FRAME_DROPPING_POLICY_H
#define MIR_COMPOSITOR_FRAME_DROPPING_POLICY_H

#include <functional>

namespace mir
{
namespace compositor
{
/**
 * \brief Policy to determine when to drop a frame from a client
 */
class FrameDroppingPolicy
{
public:
    class Cookie;

    virtual ~FrameDroppingPolicy() = default;

    // TODO: Really we want something like observing_ptr<Cookie> here
    //       but a raw ptr is ok.
    /**
     * \brief Notify that a client is now blocked on a swap
     * \param drop_frame    Function to call should a frame drop be required
     * \return              An opaque handle identifying this swap
     */
    virtual Cookie* swap_now_blocking(std::function<void(void)> drop_frame) = 0;
    /**
     * \brief Notify that a client is no longer blocked on a swap
     * \param cookie    The handle returned from swap_now_blocking
     */
    virtual void swap_unblocked(Cookie* cookie) = 0;
};

}
}

#endif // MIR_COMPOSITOR_FRAME_DROPPING_POLICY_H
