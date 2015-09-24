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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EVENT_SINK_H_
#define MIR_TEST_DOUBLES_MOCK_EVENT_SINK_H_

#include "mir/frontend/event_sink.h"
#include "mir/graphics/display_configuration.h"
#include "mir/events/event_private.h"
#include "mir/graphics/display_configuration.h"

#include <gmock/gmock.h>

namespace mir
{
namespace frontend
{
class MessageSender;
}

namespace test
{
namespace doubles
{
struct MockEventSink : public frontend::EventSink
{
    MOCK_METHOD1(handle_event, void(MirEvent const&));
    MOCK_METHOD1(handle_lifecycle_event, void(MirLifecycleState));
    MOCK_METHOD1(handle_display_config_change, void(graphics::DisplayConfiguration const&));
    MOCK_METHOD1(send_ping, void(int32_t));
    MOCK_METHOD3(send_buffer, void(frontend::BufferStreamId, graphics::Buffer&, graphics::BufferIpcMsgType));
};

class GloballyUniqueMockEventSink : public frontend::EventSink
{
public:
    GloballyUniqueMockEventSink();
    ~GloballyUniqueMockEventSink() override;

    void handle_event(MirEvent const& ev) override;
    void handle_lifecycle_event(MirLifecycleState state) override;
    void handle_display_config_change(graphics::DisplayConfiguration const& conf) override;
    void send_ping(int32_t serial) override;
    void send_buffer(frontend::BufferStreamId id, graphics::Buffer& buf, graphics::BufferIpcMsgType type) override;

    MockEventSink* as_mock() const;

private:
    std::shared_ptr<MockEventSink> underlying_sink;
    static std::weak_ptr<MockEventSink> global_sink;
};

std::unique_ptr<frontend::EventSink> mock_sink_factory(std::shared_ptr<frontend::MessageSender> const&);
std::shared_ptr<MockEventSink> the_mock_sink();
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_EVENT_SINK_H_*/
