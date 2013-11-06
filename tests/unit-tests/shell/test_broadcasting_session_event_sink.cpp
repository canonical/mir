/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/shell/broadcasting_session_event_sink.h"
#include "mir_test_doubles/stub_shell_session.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>

namespace msh = mir::shell;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

TEST(BroadcastingSessionEventSinkTest, emits_and_handles_focus_change)
{
    mtd::StubShellSession session1;
    std::vector<msh::Session*> handler_called(3, nullptr);

    msh::BroadcastingSessionEventSink events;

    for (auto& h : handler_called)
    {
        events.register_focus_change_handler(
            [&h](std::shared_ptr<msh::Session> const& session)
            {
                h = session.get();
            });
    }

    events.handle_focus_change(mt::fake_shared(session1));

    for (unsigned int i = 0; i < handler_called.size(); i++)
    {
        EXPECT_EQ(&session1, handler_called[i]) << " i = " << i;
    }
}

TEST(BroadcastingSessionEventSinkTest, emits_and_handles_no_focus)
{
    mtd::StubShellSession session1;
    std::vector<int> handler_called(3, 0);

    msh::BroadcastingSessionEventSink events;

    for (auto& h : handler_called)
    {
        events.register_no_focus_handler(
            [&h]
            {
                h = 1;
            });
    }

    events.handle_no_focus();

    for (unsigned int i = 0; i < handler_called.size(); i++)
    {
        EXPECT_EQ(1, handler_called[i]) << " i = " << i;
    }
}

TEST(BroadcastingSessionEventSinkTest, emits_and_handles_session_stopping)
{
    mtd::StubShellSession session1;
    std::vector<msh::Session*> handler_called(3, nullptr);

    msh::BroadcastingSessionEventSink events;

    for (auto& h : handler_called)
    {
        events.register_session_stopping_handler(
            [&h](std::shared_ptr<msh::Session> const& session)
            {
                h = session.get();
            });
    }

    events.handle_session_stopping(mt::fake_shared(session1));

    for (unsigned int i = 0; i < handler_called.size(); i++)
    {
        EXPECT_EQ(&session1, handler_called[i]) << " i = " << i;
    }
}
