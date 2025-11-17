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

#include "src/server/scene/broadcasting_session_event_sink.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
using namespace testing;

namespace
{
struct MockEventSink : ms::SessionEventSink
{
    MOCK_METHOD(void, handle_focus_change, (std::shared_ptr<ms::Session> const& session), (override));
    MOCK_METHOD(void, handle_session_stopping, (std::shared_ptr<ms::Session> const& session), (override));
    MOCK_METHOD(void, handle_no_focus, (), (override));
};
}

TEST(BroadcastingSessionEventSinkTest, emits_and_handles_focus_change)
{
    mtd::StubSession session1;
    MockEventSink handler_called[3];

    ms::BroadcastingSessionEventSink events;

    std::shared_ptr<ms::Session> session1ptr{mt::fake_shared(session1)};

    for (auto& h : handler_called)
    {
        events.add(&h);

        EXPECT_CALL(h, handle_focus_change(session1ptr)).Times(1);
    }

    events.handle_focus_change(session1ptr);
}

TEST(BroadcastingSessionEventSinkTest, emits_and_handles_no_focus)
{
    mtd::StubSession session1;
    MockEventSink handler_called[3];

    ms::BroadcastingSessionEventSink events;

    for (auto& h : handler_called)
    {
        events.add(&h);

        EXPECT_CALL(h, handle_no_focus()).Times(1);
    }

    events.handle_no_focus();
}

TEST(BroadcastingSessionEventSinkTest, emits_and_handles_session_stopping)
{
    mtd::StubSession session1;
    MockEventSink handler_called[3];

    ms::BroadcastingSessionEventSink events;

    std::shared_ptr<ms::Session> session1ptr{mt::fake_shared(session1)};

    for (auto& h : handler_called)
    {
        events.add(&h);

        EXPECT_CALL(h, handle_session_stopping(session1ptr)).Times(1);
    }

    events.handle_session_stopping(session1ptr);
}
