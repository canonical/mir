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

#ifndef MIR_EVENT_QUEUE_H_
#define MIR_EVENT_QUEUE_H_

#include "mir_toolkit/event.h"
#include "mir/event_sink.h"
#include <memory>

namespace mir
{
class EventQueue : public EventSink
{
public:
    void set_target(std::weak_ptr<EventSink> const& s);
    void handle_event(MirEvent const& e) override;

private:
    std::weak_ptr<EventSink> target;
};

} // namespace mir

#endif
