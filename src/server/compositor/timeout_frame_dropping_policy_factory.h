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


#ifndef MIR_COMPOSITOR_TIMEOUT_FRAME_DROPPING_POLICY_FACTORY_H_
#define MIR_COMPOSITOR_TIMEOUT_FRAME_DROPPING_POLICY_FACTORY_H_

#include "mir/compositor/frame_dropping_policy_factory.h"
#include "mir/time/alarm_factory.h"

#include <memory>
#include <chrono>

namespace mir
{
namespace compositor
{

/**
 * \brief Creator of timeout-based FrameDroppingPolicies
 */
class TimeoutFrameDroppingPolicyFactory : public FrameDroppingPolicyFactory
{
public:
    /**
     * \param factory Factory which can create alarms used to schedule frame drops.
     * \param timeout Number of milliseconds to wait before dropping a frame
     */
    TimeoutFrameDroppingPolicyFactory(
        std::shared_ptr<mir::time::AlarmFactory> const& factory,
        std::chrono::milliseconds timeout);

    std::unique_ptr<FrameDroppingPolicy> create_policy(
        std::shared_ptr<LockableCallback> const& drop_frame) const override;
private:
    std::shared_ptr<mir::time::AlarmFactory> const factory;
    std::chrono::milliseconds timeout;
};

}
}

#endif // MIR_COMPOSITOR_TIMEOUT_FRAME_DROPPING_POLICY_FACTORY_FACTORY_H_
