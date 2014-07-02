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


#ifndef MIR_TEST_DOUBLES_MOCK_FRAME_DROPPING_POLICY_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_FRAME_DROPPING_POLICY_FACTORY_H_

#include "mir/compositor/frame_dropping_policy_factory.h"
#include "mir/compositor/frame_dropping_policy.h"

#include <unordered_set>
#include <functional>

#include <gmock/gmock.h>

namespace mc = mir::compositor;

namespace mir
{
namespace test
{
namespace doubles
{

class MockFrameDroppingPolicyFactory;

class MockFrameDroppingPolicy : public mc::FrameDroppingPolicy
{
public:
    MockFrameDroppingPolicy(std::function<void(void)> callback,
                            MockFrameDroppingPolicyFactory const* parent);
    ~MockFrameDroppingPolicy();

    MOCK_METHOD0(swap_now_blocking, void(void));
    MOCK_METHOD0(swap_unblocked, void(void));

    void trigger();

private:
    friend class MockFrameDroppingPolicyFactory;
    void parent_destroyed();

    std::function<void(void)> callback;
    MockFrameDroppingPolicyFactory const* parent;
};

class MockFrameDroppingPolicyFactory : public mc::FrameDroppingPolicyFactory
{
public:
    std::unique_ptr<mc::FrameDroppingPolicy> create_policy(std::function<void(void)> drop_frame) const override;

    ~MockFrameDroppingPolicyFactory();

    void trigger_policies() const;

private:
    friend class MockFrameDroppingPolicy;

    void policy_destroyed(MockFrameDroppingPolicy* policy) const;
    mutable std::unordered_set<MockFrameDroppingPolicy*> policies;
};

}
}
}


#endif // MIR_TEST_DOUBLES_MOCK_FRAME_DROPPING_POLICY_FACTORY_H_
