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


#ifndef MIR_TEST_DOUBLES_STUB_FRAME_DROPPING_POLICY_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_FRAME_DROPPING_POLICY_FACTORY_H_

#include "mir/compositor/frame_dropping_policy_factory.h"
#include "mir/compositor/frame_dropping_policy.h"

namespace mc = mir::compositor;

namespace mir
{
namespace test
{
namespace doubles
{

class StubFrameDroppingPolicy : public mc::FrameDroppingPolicy
{
public:
    void swap_now_blocking()
    {
    }
    void swap_unblocked()
    {
    }
};

class StubFrameDroppingPolicyFactory : public mc::FrameDroppingPolicyFactory
{
public:
    std::unique_ptr<mc::FrameDroppingPolicy> create_policy(
        std::shared_ptr<mir::LockableCallback> const&) const override
    {
       return std::unique_ptr<mc::FrameDroppingPolicy>{new StubFrameDroppingPolicy};
    }
};

}
}
}
#endif // TEST_DOUBLES_STUB_FRAME_DROPPING_POLICY_FACTORY_H_
