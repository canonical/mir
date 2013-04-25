/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/server/frontend/event_pipe.h"
#include <cstring>

#include <gtest/gtest.h>

using mir::frontend::EventPipe;


TEST(EventPipe, no_sink)
{
    EventPipe q;
    MirEvent e;

    e.type = mir_event_type_key;
    q.handle_event(e);

    e.type = mir_event_type_motion;
    q.handle_event(e);

    e.type = mir_event_type_surface;
    q.handle_event(e);
}

namespace
{
    class TestSink : public mir::events::EventSink
    {
    public:
        TestSink()
        {
            memset(&last_event, 0, sizeof last_event);
        }

        void handle_event(MirEvent const& e)
        {
            last_event = e;
        }

        MirEvent const& last_event_handled() const
        {
            return last_event;
        }

    private:
        MirEvent last_event;
    };
}

TEST(EventPipe, events_get_handled)
{
    EventPipe q;
    std::shared_ptr<TestSink> s(new TestSink);

    q.set_target(s);

    MirEvent e;
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_key;
    q.handle_event(e);
    EXPECT_EQ(mir_event_type_key, s->last_event_handled().type);

    e.type = mir_event_type_motion;
    q.handle_event(e);
    EXPECT_EQ(mir_event_type_motion, s->last_event_handled().type);

    e.type = mir_event_type_surface;
    q.handle_event(e);
    EXPECT_EQ(mir_event_type_surface, s->last_event_handled().type);
}

TEST(EventPipe, sink_is_changeable)
{
    EventPipe q;
    std::shared_ptr<TestSink> a(new TestSink);
    std::shared_ptr<TestSink> b(new TestSink);

    q.set_target(a);

    MirEvent e;
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_motion;
    q.handle_event(e);
    EXPECT_EQ(mir_event_type_motion, a->last_event_handled().type);

    q.set_target(b);

    e.type = mir_event_type_surface;
    q.handle_event(e);
    EXPECT_EQ(mir_event_type_surface, b->last_event_handled().type);
    EXPECT_EQ(mir_event_type_motion, a->last_event_handled().type);

    e.type = mir_event_type_key;
    q.handle_event(e);
    EXPECT_EQ(mir_event_type_key, b->last_event_handled().type);
    EXPECT_EQ(mir_event_type_motion, a->last_event_handled().type);
}
