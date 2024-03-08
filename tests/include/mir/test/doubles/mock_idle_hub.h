/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_IDLE_HUB_H_
#define MIR_TEST_DOUBLES_MOCK_IDLE_HUB_H_

#include "mir/scene/idle_hub.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockIdleHub : mir::scene::IdleHub
{
    MOCK_METHOD(void, poke, (), (override));
    MOCK_METHOD(void, register_interest, (std::weak_ptr<mir::scene::IdleStateObserver> const&, time::Duration), (override));
    MOCK_METHOD(void, register_interest, (
        std::weak_ptr<mir::scene::IdleStateObserver> const&,
        mir::NonBlockingExecutor&,
        time::Duration), (override));
    MOCK_METHOD(void, unregister_interest, (mir::scene::IdleStateObserver const&), (override));
    MOCK_METHOD(std::shared_ptr<IdleHub::WakeLock>, inhibit_idle, (), (override));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_IDLE_HUB_H_
