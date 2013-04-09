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

#include <gtest/gtest.h>
#include "mir/event_queue.h"
#include "mir/event_sink.h"
#include <cstring>

TEST(EventQueue, no_sink)
{
    EXPECT_NO_FATAL_FAILURE({
        mir::EventQueue q;
        MirEvent e;

        e.type = mir_event_type_key;
        q.post(e);

        e.type = mir_event_type_motion;
        q.post(e);

        e.type = mir_event_type_hw_switch;
        q.post(e);

        e.type = mir_event_type_surface_change;
        q.post(e);
    });
}

namespace
{
    class TestSink : public mir::EventSink
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

TEST(EventQueue, events_get_handled)
{
    mir::EventQueue q;
    std::shared_ptr<TestSink> s(new TestSink);

    q.set_sink(s);

    MirEvent e;
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_key;
    q.post(e);
    EXPECT_EQ(mir_event_type_key, s->last_event_handled().type);

    e.type = mir_event_type_motion;
    q.post(e);
    EXPECT_EQ(mir_event_type_motion, s->last_event_handled().type);

    e.type = mir_event_type_hw_switch;
    q.post(e);
    EXPECT_EQ(mir_event_type_hw_switch, s->last_event_handled().type);

    e.type = mir_event_type_surface_change;
    q.post(e);
    EXPECT_EQ(mir_event_type_surface_change, s->last_event_handled().type);
}

TEST(EventQueue, sink_is_changeable)
{
    mir::EventQueue q;
    std::shared_ptr<TestSink> a(new TestSink);
    std::shared_ptr<TestSink> b(new TestSink);

    q.set_sink(a);

    MirEvent e;
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_motion;
    q.post(e);
    EXPECT_EQ(mir_event_type_motion, a->last_event_handled().type);

    e.type = mir_event_type_hw_switch;
    q.post(e);
    EXPECT_EQ(mir_event_type_hw_switch, a->last_event_handled().type);

    q.set_sink(b);

    e.type = mir_event_type_surface_change;
    q.post(e);
    EXPECT_EQ(mir_event_type_surface_change, b->last_event_handled().type);
    EXPECT_EQ(mir_event_type_hw_switch, a->last_event_handled().type);

    e.type = mir_event_type_key;
    q.post(e);
    EXPECT_EQ(mir_event_type_key, b->last_event_handled().type);
    EXPECT_EQ(mir_event_type_hw_switch, a->last_event_handled().type);
}
