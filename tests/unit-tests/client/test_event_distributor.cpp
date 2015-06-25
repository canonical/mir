/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Nick Dedekind <nick.dedekind <nick.dedekind@canonical.com>
 */

#include "src/client/mir_event_distributor.h"

#include "mir/events/event_private.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/test/wait_condition.h>

#include <thread>

namespace mcl = mir::client;
namespace mt = mir::test;

namespace
{

class EventDistributorTest : public testing::Test
{
public:
    MOCK_METHOD1(event_handled1, void(MirEvent const&));
    MOCK_METHOD1(event_handled2, void(MirEvent const&));

    MirEventDistributor event_distributor;
};

MATCHER_P(MirEventTypeIs, type, "")
{
    return (arg.type == type);
}

}

TEST_F(EventDistributorTest, calls_back_when_registered)
{
    using namespace testing;

    event_distributor.register_event_handler([this](MirEvent const& event) { event_handled1(event); });
    event_distributor.register_event_handler([this](MirEvent const& event) { event_handled2(event); });

    MirEvent e;
    e.type = mir_event_type_prompt_session_state_change;

    EXPECT_CALL(*this, event_handled1(MirEventTypeIs(e.type))).Times(1);
    EXPECT_CALL(*this, event_handled2(MirEventTypeIs(e.type))).Times(1);
    event_distributor.handle_event(e);
}

TEST_F(EventDistributorTest, no_calls_back_after_unregistered)
{
    using namespace testing;

    event_distributor.register_event_handler([this](MirEvent const& event) { event_handled1(event); });
    int reg_id2 = event_distributor.register_event_handler([this](MirEvent const& event) { event_handled2(event); });
    event_distributor.unregister_event_handler(reg_id2);

    MirEvent e;
    e.type = mir_event_type_prompt_session_state_change;

    EXPECT_CALL(*this, event_handled1(MirEventTypeIs(e.type))).Times(1);
    EXPECT_CALL(*this, event_handled2(MirEventTypeIs(e.type))).Times(0);
    event_distributor.handle_event(e);
}

TEST_F(EventDistributorTest, no_callback_on_callback_deregistration)
{
    using namespace testing;
    int reg_id2;

    event_distributor.register_event_handler(
        [this, &reg_id2](MirEvent const& event)
        {
            event_handled1(event);
            event_distributor.unregister_event_handler(reg_id2);
        });
    reg_id2 = event_distributor.register_event_handler([this](MirEvent const& event) { event_handled2(event); });

    MirEvent e;
    e.type = mir_event_type_prompt_session_state_change;

    EXPECT_CALL(*this, event_handled1(MirEventTypeIs(e.type))).Times(1);
    EXPECT_CALL(*this, event_handled2(MirEventTypeIs(e.type))).Times(0);
    event_distributor.handle_event(e);
}

TEST_F(EventDistributorTest, succeeds_with_thread_delete_unregister)
{
    using namespace testing;

    struct EventCatcher
    {
        EventCatcher(mcl::EventDistributor* event_distributor)
        : event_distributor(event_distributor)
        {
            locked = false;
            reg = event_distributor->register_event_handler(
                [this](MirEvent const&)
                {
                    if (locked)
                    {
                        locked = false;
                        mutex.unlock();
                    }
                });
            mutex.lock();
            locked = true;
        }
        ~EventCatcher()
        {
            std::unique_lock<std::mutex> lk(mutex);
            event_distributor->unregister_event_handler(reg);
        }

        mcl::EventDistributor* event_distributor;
        int reg;
        bool locked;
        std::mutex mutex;
    };

    MirEvent e;
    e.type = mir_event_type_prompt_session_state_change;

    std::vector<EventCatcher*> catchers;
    for (int p = 0; p < 10; p++)
    {
        catchers.push_back(new EventCatcher(&event_distributor));
    }

    mt::WaitCondition thread_done;
    auto thread = std::thread{
        [&]
        {
            for (auto catcher : catchers)
                delete catcher;
            thread_done.wake_up_everyone();
        }};

    while(!thread_done.woken()) {
        event_distributor.handle_event(e);
        std::this_thread::yield();
    }

    thread.join();
}

