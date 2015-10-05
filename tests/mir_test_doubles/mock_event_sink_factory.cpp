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

#include "mir/test/doubles/mock_event_sink_factory.h"

#include "mir/frontend/event_sink.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/graphics/buffer.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
class GloballyUniqueMockEventSink : public mf::EventSink
{
public:
    GloballyUniqueMockEventSink(std::shared_ptr<mf::EventSink> const& underlying_sink);
    ~GloballyUniqueMockEventSink() override;

    void handle_event(MirEvent const& ev) override;
    void handle_lifecycle_event(MirLifecycleState state) override;
    void handle_display_config_change(mg::DisplayConfiguration const& conf) override;
    void send_ping(int32_t serial) override;
    void send_buffer(mf::BufferStreamId id, mg::Buffer& buf, mg::BufferIpcMsgType type) override;

private:
    std::shared_ptr<mf::EventSink> underlying_sink;
};
}

GloballyUniqueMockEventSink::GloballyUniqueMockEventSink(std::shared_ptr<mf::EventSink> const& underlying_sink)
    : underlying_sink{underlying_sink}
{
}

GloballyUniqueMockEventSink::~GloballyUniqueMockEventSink()
{
}

void GloballyUniqueMockEventSink::handle_event(MirEvent const& ev)
{
    underlying_sink->handle_event(ev);
}

void GloballyUniqueMockEventSink::handle_lifecycle_event(MirLifecycleState state)
{
    underlying_sink->handle_lifecycle_event(state);
}

void GloballyUniqueMockEventSink::handle_display_config_change(
    mg::DisplayConfiguration const& conf)
{
    underlying_sink->handle_display_config_change(conf);
}

void GloballyUniqueMockEventSink::send_ping(int32_t serial)
{
    underlying_sink->send_ping(serial);
}

void GloballyUniqueMockEventSink::send_buffer(
    mf::BufferStreamId id,
    mg::Buffer& buf,
    mg::BufferIpcMsgType type)
{
    underlying_sink->send_buffer(id, buf, type);
}

mtd::MockEventSinkFactory::MockEventSinkFactory()
    : underlying_sink{std::make_shared<testing::NiceMock<mtd::MockEventSink>>()}
{
}

std::unique_ptr<mf::EventSink>
mtd::MockEventSinkFactory::create_sink(std::shared_ptr<mf::MessageSender> const&)
{
    return std::make_unique<GloballyUniqueMockEventSink>(underlying_sink);
}

std::shared_ptr<mtd::MockEventSink> mtd::MockEventSinkFactory::the_mock_sink()
{
    return underlying_sink;
}
