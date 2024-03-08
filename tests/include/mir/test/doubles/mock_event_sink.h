/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EVENT_SINK_H_
#define MIR_TEST_DOUBLES_MOCK_EVENT_SINK_H_

#include "mir/frontend/event_sink.h"
#include "mir/client_visible_error.h"
#include "mir/graphics/display_configuration.h"
#include "mir/events/event_private.h"
#include "mir/input/mir_input_config.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockEventSink : public frontend::EventSink
{
    // relay function to handle gmock not having support for move only types
    void handle_event(EventUPtr&& event)
    {
        handle_event(*event);
    }

    MOCK_METHOD(void, handle_event, (MirEvent const&));
    MOCK_METHOD(void, handle_lifecycle_event, (MirLifecycleState), (override));
    MOCK_METHOD(void, handle_display_config_change, (graphics::DisplayConfiguration const&), (override));
    MOCK_METHOD(void, handle_error, (ClientVisibleError const&), (override));
    MOCK_METHOD(void, send_ping, (int32_t), (override));
    MOCK_METHOD(void, send_buffer, (frontend::BufferStreamId, graphics::Buffer&), (override));
    MOCK_METHOD(void, add_buffer, (graphics::Buffer&), (override));
    MOCK_METHOD(void, update_buffer, (graphics::Buffer&), (override));
    MOCK_METHOD(void, error_buffer, (geometry::Size, MirPixelFormat, std::string const&), (override));
    MOCK_METHOD(void, handle_input_config_change, (MirInputConfig const&), (override));
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_EVENT_SINK_H_*/
