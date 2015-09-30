/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_EVENT_SINK_FACTORY_H_
#define MIR_FRONTEND_EVENT_SINK_FACTORY_H_

#include <memory>

namespace mir
{
namespace frontend
{
class EventSink;
class MessageSender;

class EventSinkFactory
{
public:
    EventSinkFactory() = default;
    virtual ~EventSinkFactory() = default;

    EventSinkFactory(EventSinkFactory const&) = delete;
    EventSinkFactory& operator=(EventSinkFactory const&) = delete;

    virtual std::unique_ptr<EventSink>
        create_sink(std::shared_ptr<MessageSender> const& sender) = 0;
};

}
}

#endif //MIR_FRONTEND_EVENT_SINK_FACTORY_H_
