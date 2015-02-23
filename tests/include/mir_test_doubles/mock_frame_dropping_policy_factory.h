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


#ifndef MIR_TEST_DOUBLES_MOCK_FRAME_DROPPING_POLICY_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_FRAME_DROPPING_POLICY_FACTORY_H_

#include "mir/compositor/frame_dropping_policy_factory.h"
#include "mir/compositor/frame_dropping_policy.h"

#include "mir_test/gmock_fixes.h"

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
    MockFrameDroppingPolicy(std::function<void()> const& callback,
                            std::function<void()> const& lock,
                            std::function<void()> const& unlock,
                            MockFrameDroppingPolicyFactory const* parent);
    ~MockFrameDroppingPolicy();

    MOCK_METHOD0(swap_now_blocking, void(void));
    MOCK_METHOD0(swap_unblocked, void(void));

    void trigger();

private:
    friend class MockFrameDroppingPolicyFactory;
    void parent_destroyed();

    std::function<void()> callback;
    std::function<void()> lock;
    std::function<void()> unlock;
    MockFrameDroppingPolicyFactory const* parent;
};

class MockFrameDroppingPolicyFactory : public mc::FrameDroppingPolicyFactory
{
public:
    std::unique_ptr<mc::FrameDroppingPolicy> create_policy(
        std::function<void()> const& drop_frame,
        std::function<void()> const& lock,
        std::function<void()> const& unlock) const override;

    ~MockFrameDroppingPolicyFactory();

    void trigger_policies() const;

private:
    friend class MockFrameDroppingPolicy;

    void policy_destroyed(MockFrameDroppingPolicy* policy) const;
    mutable std::unordered_set<MockFrameDroppingPolicy*> policies;
};

class FrameDroppingPolicyFactoryMock : public mc::FrameDroppingPolicyFactory
{
public:
    MOCK_CONST_METHOD3(create_policy, std::unique_ptr<mc::FrameDroppingPolicy>(
                     std::function<void()> const&,
                     std::function<void()> const&,
                     std::function<void()> const&));
};

}
}
}


#endif // MIR_TEST_DOUBLES_MOCK_FRAME_DROPPING_POLICY_FACTORY_H_
