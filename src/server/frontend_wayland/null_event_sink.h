/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_NULL_EVENT_SINK_H_
#define MIR_FRONTEND_NULL_EVENT_SINK_H_

#include "mir/frontend/event_sink.h"

namespace mir
{
namespace frontend
{
class NullEventSink : public EventSink
{
public:
    NullEventSink() {}

    void handle_event(EventUPtr&& event) override;

    void handle_lifecycle_event(MirLifecycleState state) override;

    void handle_display_config_change(graphics::DisplayConfiguration const& config) override;

    void send_ping(int32_t serial) override;

    void send_buffer(BufferStreamId id, graphics::Buffer& buffer, graphics::BufferIpcMsgType type) override;

    void handle_input_config_change(MirInputConfig const&) override;

    void handle_error(ClientVisibleError const&) override;

    void add_buffer(graphics::Buffer&) override;

    void error_buffer(geometry::Size, MirPixelFormat, std::string const&) override;

    void update_buffer(graphics::Buffer&) override;
};
}
}

#endif //MIR_FRONTEND_NULL_EVENT_SINK_H_
