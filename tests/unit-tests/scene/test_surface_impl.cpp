/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"
#include "src/server/scene/basic_surface.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface_event_source.h"
#include "src/server/report/null_report_factory.h"
#include "mir/frontend/event_sink.h"
#include "mir/graphics/display_configuration.h"
#include "mir/scene/output_properties_cache.h"

#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"

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
namespace mev = mir::events;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mr = mir::report;

namespace
{

MATCHER_P(MirResizeEventEq, event, "")
{
    if (arg.type() != mir_event_type_resize ||
        event->type() != mir_event_type_resize)
    {
        return false;
    }

    return arg.to_resize()->width() == event->to_resize()->width() &&
           arg.to_resize()->height() == event->to_resize()->height() &&
           arg.to_resize()->surface_id() == event->to_resize()->surface_id();
}

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
        
        surface = std::make_shared<ms::BasicSurface>(
            std::string("stub"), geom::Rectangle{{},{}},
            mir_pointer_unconfined,
            std::list<ms::StreamInfo> { { buffer_stream, {}, {} } },
            nullptr, report);
    }

    mf::SurfaceId stub_id;
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    std::shared_ptr<ms::BasicSurface> surface;
};
}

TEST_F(Surface, attributes)
{
    using namespace testing;

    EXPECT_THROW({
        surface->configure(static_cast<MirWindowAttrib>(111), 222);
    }, std::logic_error);
}

TEST_F(Surface, types)
{
    using namespace testing;

    EXPECT_EQ(mir_window_type_normal, surface->type());

    EXPECT_EQ(mir_window_type_utility,
              surface->configure(mir_window_attrib_type,
                             mir_window_type_utility));
    EXPECT_EQ(mir_window_type_utility, surface->type());

    EXPECT_THROW({
        surface->configure(mir_window_attrib_type, 999);
    }, std::logic_error);
    EXPECT_THROW({
        surface->configure(mir_window_attrib_type, -1);
    }, std::logic_error);
    EXPECT_EQ(mir_window_type_utility, surface->type());

    EXPECT_EQ(mir_window_type_dialog,
              surface->configure(mir_window_attrib_type,
                             mir_window_type_dialog));
    EXPECT_EQ(mir_window_type_dialog, surface->type());

    EXPECT_EQ(mir_window_type_freestyle,
              surface->configure(mir_window_attrib_type,
                             mir_window_type_freestyle));
    EXPECT_EQ(mir_window_type_freestyle, surface->type());
}

TEST_F(Surface, states)
{
    using namespace testing;

    EXPECT_EQ(mir_window_state_restored, surface->state());

    EXPECT_EQ(mir_window_state_vertmaximized,
              surface->configure(mir_window_attrib_state,
                             mir_window_state_vertmaximized));
    EXPECT_EQ(mir_window_state_vertmaximized, surface->state());

    EXPECT_THROW({
        surface->configure(mir_window_attrib_state, 999);
    }, std::logic_error);
    EXPECT_THROW({
        surface->configure(mir_window_attrib_state, -1);
    }, std::logic_error);
    EXPECT_EQ(mir_window_state_vertmaximized, surface->state());

    EXPECT_EQ(mir_window_state_minimized,
              surface->configure(mir_window_attrib_state,
                             mir_window_state_minimized));
    EXPECT_EQ(mir_window_state_minimized, surface->state());

    EXPECT_EQ(mir_window_state_fullscreen,
              surface->configure(mir_window_attrib_state,
                             mir_window_state_fullscreen));
    EXPECT_EQ(mir_window_state_fullscreen, surface->state());
}

TEST_F(Surface, clamps_undersized_resize)
{
    using namespace testing;

    geom::Size const try_size{-123, -456};
    geom::Size const expect_size{1, 1};

    surface->resize(try_size);
    EXPECT_EQ(expect_size, surface->window_size());
}

TEST_F(Surface, emits_resize_events)
{
    using namespace testing;

    geom::Size const new_size{123, 456};
    auto sink = std::make_shared<mtd::MockEventSink>();
    ms::OutputPropertiesCache cache;
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, cache, sink);

    surface->add_observer(observer);

    auto e = mev::make_event(stub_id, new_size);
    EXPECT_CALL(*sink, handle_event(MirResizeEventEq(e.get())))
        .Times(1);

    surface->resize(new_size);
    EXPECT_EQ(new_size, surface->window_size());
}

TEST_F(Surface, emits_resize_events_only_on_change)
{
    using namespace testing;

    geom::Size const new_size{123, 456};
    geom::Size const new_size2{789, 1011};
    auto sink = std::make_shared<mtd::MockEventSink>();
    ms::OutputPropertiesCache cache;
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, cache, sink);

    surface->add_observer(observer);

    auto e = mev::make_event(stub_id, new_size);
    EXPECT_CALL(*sink, handle_event(MirResizeEventEq(e.get())))
        .Times(1);

    auto e2 = mev::make_event(stub_id, new_size2);
    EXPECT_CALL(*sink, handle_event(MirResizeEventEq(e2.get())))
        .Times(1);

    surface->resize(new_size);
    EXPECT_EQ(new_size, surface->window_size());
    surface->resize(new_size);
    EXPECT_EQ(new_size, surface->window_size());

    surface->resize(new_size2);
    EXPECT_EQ(new_size2, surface->window_size());
    surface->resize(new_size2);
    EXPECT_EQ(new_size2, surface->window_size());
}

TEST_F(Surface, sends_focus_notifications_when_focus_gained_and_lost)
{
    using namespace testing;

    mtd::MockEventSink sink;

    {
        InSequence seq;
        EXPECT_CALL(sink, handle_event(mt::WindowEvent(mir_window_attrib_focus, mir_window_focus_state_focused)))
            .Times(1);
        EXPECT_CALL(sink, handle_event(mt::WindowEvent(mir_window_attrib_focus, mir_window_focus_state_unfocused)))
            .Times(1);
    }

    ms::OutputPropertiesCache cache;
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, cache, mt::fake_shared(sink));

    surface->add_observer(observer);

    surface->configure(mir_window_attrib_focus, mir_window_focus_state_focused);
    surface->configure(mir_window_attrib_focus, mir_window_focus_state_unfocused);
}
MATCHER_P(MirCloseSurfaceEventMatches, event, "")
{
    return arg.type() == event->type();
}

TEST_F(Surface, emits_client_close_events)
{
    using namespace testing;

    auto sink = std::make_shared<mtd::MockEventSink>();
    ms::OutputPropertiesCache cache;
    auto const observer = std::make_shared<ms::SurfaceEventSource>(stub_id, cache, sink);

    surface->add_observer(observer);

    MirCloseWindowEvent e;
    e.to_close_window()->set_surface_id(stub_id.as_value());

    EXPECT_CALL(*sink, handle_event(MirCloseSurfaceEventMatches(&e))).Times(1);

    surface->request_client_surface_close();
}

TEST_F(Surface, preferred_orientation_mode_defaults_to_any)
{
    using namespace testing;

    ms::BasicSurface surf(
        std::string("stub"),
        geom::Rectangle{{},{}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { buffer_stream, {}, {} } },
        std::shared_ptr<mg::CursorImage>(),
        report);

    EXPECT_EQ(mir_orientation_mode_any, surf.query(mir_window_attrib_preferred_orientation));
}
