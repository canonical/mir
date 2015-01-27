/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "src/server/scene/basic_surface.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface_event_source.h"
#include "src/server/report/null_report_factory.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/display_configuration.h"

#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_input_targeter.h"
#include "mir_test_doubles/stub_input_sender.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_surface_configurator.h"
#include "mir_test_doubles/mock_surface_configurator.h"
#include "mir_test/fake_shared.h"
#include "mir_test/event_matchers.h"

#include <cstring>
#include <stdexcept>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mr = mir::report;

namespace
{

struct MockEventSink : public mf::EventSink
{
    ~MockEventSink() noexcept(true) {}
    MOCK_METHOD1(handle_event, void(MirEvent const&));
    MOCK_METHOD1(handle_lifecycle_event, void(MirLifecycleState));
    MOCK_METHOD1(handle_display_config_change, void(mg::DisplayConfiguration const&));
};

typedef testing::NiceMock<mtd::MockBufferStream> StubBufferStream;

struct Surface : testing::Test
{
    std::shared_ptr<StubBufferStream> const buffer_stream = std::make_shared<StubBufferStream>();

    void SetUp()
    {
        using namespace testing;

        ON_CALL(*buffer_stream, stream_size()).WillByDefault(Return(geom::Size()));
        ON_CALL(*buffer_stream, get_stream_pixel_format()).WillByDefault(Return(mir_pixel_format_abgr_8888));
        ON_CALL(*buffer_stream, acquire_client_buffer(_))
            .WillByDefault(InvokeArgument<0>(nullptr));
        
        surface = std::make_shared<ms::BasicSurface>(std::string("stub"), geom::Rectangle{{},{}}, false,
            buffer_stream, nullptr /* input_channel */, stub_input_sender,
            null_configurator, nullptr /* cursor_image */, report);
    }

    mf::SurfaceId stub_id;
    std::shared_ptr<ms::SurfaceConfigurator> null_configurator = std::make_shared<mtd::NullSurfaceConfigurator>();
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    std::shared_ptr<mtd::StubInputSender> const stub_input_sender = std::make_shared<mtd::StubInputSender>();
    
    std::shared_ptr<ms::BasicSurface> surface;
};
}

TEST_F(Surface, attributes)
{
    using namespace testing;

    EXPECT_THROW({
        surface->configure(static_cast<MirSurfaceAttrib>(111), 222);
    }, std::logic_error);
}

TEST_F(Surface, types)
{
    using namespace testing;

    EXPECT_EQ(mir_surface_type_normal, surface->type());

    EXPECT_EQ(mir_surface_type_utility,
              surface->configure(mir_surface_attrib_type,
                             mir_surface_type_utility));
    EXPECT_EQ(mir_surface_type_utility, surface->type());

    EXPECT_THROW({
        surface->configure(mir_surface_attrib_type, 999);
    }, std::logic_error);
    EXPECT_THROW({
        surface->configure(mir_surface_attrib_type, -1);
    }, std::logic_error);
    EXPECT_EQ(mir_surface_type_utility, surface->type());

    EXPECT_EQ(mir_surface_type_dialog,
              surface->configure(mir_surface_attrib_type,
                             mir_surface_type_dialog));
    EXPECT_EQ(mir_surface_type_dialog, surface->type());

    EXPECT_EQ(mir_surface_type_freestyle,
              surface->configure(mir_surface_attrib_type,
                             mir_surface_type_freestyle));
    EXPECT_EQ(mir_surface_type_freestyle, surface->type());
}

TEST_F(Surface, states)
{
    using namespace testing;

    EXPECT_EQ(mir_surface_state_restored, surface->state());

    EXPECT_EQ(mir_surface_state_vertmaximized,
              surface->configure(mir_surface_attrib_state,
                             mir_surface_state_vertmaximized));
    EXPECT_EQ(mir_surface_state_vertmaximized, surface->state());

    EXPECT_THROW({
        surface->configure(mir_surface_attrib_state, 999);
    }, std::logic_error);
    EXPECT_THROW({
        surface->configure(mir_surface_attrib_state, -1);
    }, std::logic_error);
    EXPECT_EQ(mir_surface_state_vertmaximized, surface->state());

    EXPECT_EQ(mir_surface_state_minimized,
              surface->configure(mir_surface_attrib_state,
                             mir_surface_state_minimized));
    EXPECT_EQ(mir_surface_state_minimized, surface->state());

    EXPECT_EQ(mir_surface_state_fullscreen,
              surface->configure(mir_surface_attrib_state,
                             mir_surface_state_fullscreen));
    EXPECT_EQ(mir_surface_state_fullscreen, surface->state());
}

bool operator==(MirEvent const& a, MirEvent const& b)
{
    // We will always fill unused bytes with zero, so memcmp is accurate...
    return !memcmp(&a, &b, sizeof(MirEvent));
}

TEST_F(Surface, clamps_undersized_resize)
{
    using namespace testing;

    geom::Size const try_size{-123, -456};
    geom::Size const expect_size{1, 1};

    surface->resize(try_size);
    EXPECT_EQ(expect_size, surface->size());
}

TEST_F(Surface, emits_resize_events)
{
    using namespace testing;

    geom::Size const new_size{123, 456};
    auto sink = std::make_shared<MockEventSink>();
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, sink);

    surface->add_observer(observer);

    MirEvent e;
    memset(&e, 0, sizeof e);
    e.type = mir_event_type_resize;
    e.resize.surface_id = stub_id.as_value();
    e.resize.width = new_size.width.as_int();
    e.resize.height = new_size.height.as_int();
    EXPECT_CALL(*sink, handle_event(e))
        .Times(1);

    surface->resize(new_size);
    EXPECT_EQ(new_size, surface->size());
}

TEST_F(Surface, emits_resize_events_only_on_change)
{
    using namespace testing;

    geom::Size const new_size{123, 456};
    geom::Size const new_size2{789, 1011};
    auto sink = std::make_shared<MockEventSink>();
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, sink);

    surface->add_observer(observer);

    MirEvent e;
    memset(&e, 0, sizeof e);
    e.type = mir_event_type_resize;
    e.resize.surface_id = stub_id.as_value();
    e.resize.width = new_size.width.as_int();
    e.resize.height = new_size.height.as_int();
    EXPECT_CALL(*sink, handle_event(e))
        .Times(1);

    MirEvent e2;
    memset(&e2, 0, sizeof e2);
    e2.type = mir_event_type_resize;
    e2.resize.surface_id = stub_id.as_value();
    e2.resize.width = new_size2.width.as_int();
    e2.resize.height = new_size2.height.as_int();
    EXPECT_CALL(*sink, handle_event(e2))
        .Times(1);

    surface->resize(new_size);
    EXPECT_EQ(new_size, surface->size());
    surface->resize(new_size);
    EXPECT_EQ(new_size, surface->size());

    surface->resize(new_size2);
    EXPECT_EQ(new_size2, surface->size());
    surface->resize(new_size2);
    EXPECT_EQ(new_size2, surface->size());
}

TEST_F(Surface, sends_focus_notifications_when_focus_gained_and_lost)
{
    using namespace testing;

    MockEventSink sink;

    {
        InSequence seq;
        EXPECT_CALL(sink, handle_event(mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_focused)))
            .Times(1);
        EXPECT_CALL(sink, handle_event(mt::SurfaceEvent(mir_surface_attrib_focus, mir_surface_unfocused)))
            .Times(1);
    }

    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, mt::fake_shared(sink));

    surface->add_observer(observer);

    surface->configure(mir_surface_attrib_focus, mir_surface_focused);
    surface->configure(mir_surface_attrib_focus, mir_surface_unfocused);
}

TEST_F(Surface, configurator_selects_attribute_values)
{
    using namespace testing;

    mtd::MockSurfaceConfigurator configurator;

    EXPECT_CALL(configurator, select_attribute_value(_, mir_surface_attrib_state, mir_surface_state_restored)).Times(1)
        .WillOnce(Return(mir_surface_state_minimized));
    EXPECT_CALL(configurator, attribute_set(_, mir_surface_attrib_state, mir_surface_state_minimized)).Times(1);

    ms::BasicSurface surf(
        std::string("stub"),
        geom::Rectangle{{},{}},
        false,
        buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        mt::fake_shared(configurator),
        std::shared_ptr<mg::CursorImage>(),
        report);

    EXPECT_EQ(mir_surface_state_minimized, surf.configure(mir_surface_attrib_state, mir_surface_state_restored));
}

TEST_F(Surface, take_input_focus)
{
    using namespace ::testing;

    mtd::MockInputTargeter targeter;
    EXPECT_CALL(targeter, focus_changed(_)).Times(1);

    surface->take_input_focus(mt::fake_shared(targeter));
}

TEST_F(Surface, with_most_recent_buffer_do_uses_compositor_buffer)
{
    auto stub_buffer_stream = std::make_shared<mtd::StubBufferStream>();

    ms::BasicSurface surf(
        std::string("stub"),
        geom::Rectangle{{},{}},
        false,
        stub_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        null_configurator,
        std::shared_ptr<mg::CursorImage>(),
        report);

    mg::Buffer* buf_ptr{nullptr};

    surf.with_most_recent_buffer_do(
        [&](mg::Buffer& buffer)
        {
            buf_ptr = &buffer;
        });

    EXPECT_EQ(stub_buffer_stream->stub_compositor_buffer.get(), buf_ptr);
}

TEST_F(Surface, emits_client_close_events)
{
    using namespace testing;

    auto sink = std::make_shared<MockEventSink>();
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, sink);

    surface->add_observer(observer);

    MirEvent e;
    memset(&e, 0, sizeof e);
    e.type = mir_event_type_close_surface;
    e.close_surface.surface_id = stub_id.as_value();

    EXPECT_CALL(*sink, handle_event(e)).Times(1);

    surface->request_client_surface_close();
}

TEST_F(Surface, preferred_orientation_mode_defaults_to_any)
{
    using namespace testing;

    ms::BasicSurface surf(
        std::string("stub"),
        geom::Rectangle{{},{}},
        false,
        buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        stub_input_sender,
        null_configurator,
        std::shared_ptr<mg::CursorImage>(),
        report);

    EXPECT_EQ(mir_orientation_mode_any, surf.query(mir_surface_attrib_preferred_orientation));
}
