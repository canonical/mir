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


#ifndef MIR_COMPOSITOR_TIMEOUT_FRAME_DROPPING_POLICY_H_
#define MIR_COMPOSITOR_TIMEOUT_FRAME_DROPPING_POLICY_H_

#include "mir/compositor/frame_dropping_policy.h"
#include "mir/time/timer.h"

#include <memory>
#include <chrono>

namespace mir
{
namespace compositor
{
class TimeoutFrameDroppingPolicy : public FrameDroppingPolicy
{
public:
    TimeoutFrameDroppingPolicy(std::shared_ptr<mir::time::Timer> const& timer,
                               std::chrono::milliseconds timeout);

    Cookie* swap_now_blocking(std::function<void(void)> drop_frame) override;
    void swap_unblocked(Cookie* cookie) override;

private:
    std::shared_ptr<mir::time::Timer> const timer;
    std::unique_ptr<mir::time::Alarm> alarm;
    std::chrono::milliseconds const timeout;
};
}
}

#endif // MIR_COMPOSITOR_TIMEOUT_FRAME_DROPPING_POLICY_H_
