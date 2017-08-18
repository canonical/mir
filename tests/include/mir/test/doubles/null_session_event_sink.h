/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_SESSION_EVENT_SINK_H_
#define MIR_TEST_DOUBLES_NULL_SESSION_EVENT_SINK_H_

#include "src/include/server/mir/scene/session_event_sink.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullSessionEventSink : public scene::SessionEventSink
{
public:
    void handle_focus_change(std::shared_ptr<scene::Session> const&) {}
    void handle_no_focus() {}
    void handle_session_stopping(std::shared_ptr<scene::Session> const&) {}
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_SESSION_EVENT_SINK_H_*/
