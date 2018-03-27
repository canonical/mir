/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_FRONTEND_EVENT_SINK_H_
#define MIR_FRONTEND_EVENT_SINK_H_

#include "mir_toolkit/event.h"
#include "mir/frontend/buffer_sink.h"
#include "mir/events/event_builders.h"

#include <vector>

class MirInputConfig;
namespace mir
{
class ClientVisibleError;
namespace graphics
{
class DisplayConfiguration;
class Buffer;
}
namespace frontend
{
class EventSink : public BufferSink
{
public:
    virtual ~EventSink() = default;

    virtual void handle_event(EventUPtr&& event) = 0;
    virtual void handle_lifecycle_event(MirLifecycleState state) = 0;
    virtual void handle_display_config_change(graphics::DisplayConfiguration const& config) = 0;
    virtual void send_ping(int32_t serial) = 0;
    virtual void handle_input_config_change(MirInputConfig const& config) = 0;
    virtual void handle_error(ClientVisibleError const& error) = 0;

protected:
    EventSink() = default;
    EventSink(EventSink const&) = delete;
    EventSink& operator=(EventSink const&) = delete;
};
}
} // namespace mir

#endif // MIR_FRONTEND_EVENT_SINK_H_
