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

#include "mir/graphics/buffer.h"
#include "mir/test/doubles/mock_event_sink.h"

namespace mtd = mir::test::doubles;

mtd::GloballyUniqueMockEventSink::GloballyUniqueMockEventSink()
    : underlying_sink{global_sink.lock()}
{
    if (!underlying_sink)
    {
        underlying_sink = std::make_shared<testing::NiceMock<MockEventSink>>();
    }
    global_sink = underlying_sink;
}

mtd::GloballyUniqueMockEventSink::~GloballyUniqueMockEventSink()
{
}

void mtd::GloballyUniqueMockEventSink::handle_event(MirEvent const& ev)
{
    underlying_sink->handle_event(ev);
}

void mtd::GloballyUniqueMockEventSink::handle_lifecycle_event(MirLifecycleState state)
{
    underlying_sink->handle_lifecycle_event(state);
}

void mtd::GloballyUniqueMockEventSink::handle_display_config_change(
    graphics::DisplayConfiguration const& conf)
{
    underlying_sink->handle_display_config_change(conf);
}

void mtd::GloballyUniqueMockEventSink::send_ping(int32_t serial)
{
    underlying_sink->send_ping(serial);
}

void mtd::GloballyUniqueMockEventSink::send_buffer(
    frontend::BufferStreamId id,
    graphics::Buffer& buf,
    graphics::BufferIpcMsgType type)
{
    underlying_sink->send_buffer(id, buf, type);
}

mtd::MockEventSink* mtd::GloballyUniqueMockEventSink::as_mock() const
{
    return underlying_sink.get();
}

std::weak_ptr<mtd::MockEventSink> mtd::GloballyUniqueMockEventSink::global_sink;

std::unique_ptr<mir::frontend::EventSink> mtd::mock_sink_factory(std::shared_ptr<mir::frontend::MessageSender> const&)
{
    return std::make_unique<mtd::GloballyUniqueMockEventSink>();
}

std::shared_ptr<mtd::MockEventSink> mtd::the_mock_sink()
{
    auto sink = std::make_shared<mtd::GloballyUniqueMockEventSink>();
    return std::shared_ptr<mtd::MockEventSink>{sink, sink->as_mock()};
}
