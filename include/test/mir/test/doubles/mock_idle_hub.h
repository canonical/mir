/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_IDLE_HUB_H_
#define MIR_TEST_DOUBLES_MOCK_IDLE_HUB_H_

#include "mir/scene/idle_hub.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct MockIdleHub : mir::scene::IdleHub
{
    MOCK_METHOD0(state, mir::scene::IdleState());
    MOCK_METHOD0(poke, void());
    MOCK_METHOD1(register_interest, void(std::weak_ptr<mir::scene::IdleStateObserver> const&));
    MOCK_METHOD2(register_interest, void(std::weak_ptr<mir::scene::IdleStateObserver> const&, mir::Executor&));
    MOCK_METHOD1(unregister_interest, void(mir::scene::IdleStateObserver const&));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_IDLE_HUB_H_
