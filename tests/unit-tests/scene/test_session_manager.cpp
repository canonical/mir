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

#include "src/server/scene/session_manager.h"

#include "mir/scene/session.h"
#include "mir/scene/session_container.h"
#include "mir/scene/null_session_listener.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/scene/null_surface_observer.h"
#include "src/server/scene/basic_surface.h"
#include "src/include/server/mir/scene/session_event_sink.h"
#include "src/server/report/null_report_factory.h"

#include "mir/test/doubles/mock_surface_stack.h"
#include "mir/test/doubles/mock_session_listener.h"
#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir/test/doubles/null_session_event_sink.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/stub_surface_factory.h"
#include "mir/test/doubles/null_application_not_responding_detector.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_observer_registrar.h"

#include "mir/test/fake_shared.h"
#include "mir/test/make_surface_spec.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockSessionEventSink : public ms::SessionEventSink
{
    MOCK_METHOD1(handle_focus_change, void(std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD0(handle_no_focus, void());
    MOCK_METHOD1(handle_session_stopping, void(std::shared_ptr<ms::Session> const& session));
};

struct SessionManagerSetup : public testing::Test
{
    std::shared_ptr<ms::Surface> dummy_surface = std::make_shared<ms::BasicSurface>(
        nullptr /* session */,
        mw::Weak<mf::WlSurface>{},
        std::string("stub"),
        geom::Rectangle{{},{}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo> { { std::make_shared<mtd::StubBufferStream>(), {}} },
        std::shared_ptr<mg::CursorImage>(),
        mir::report::null_scene_report(),
        std::make_shared<mtd::FakeDisplayConfigurationObserverRegistrar>());
    testing::NiceMock<mtd::MockSurfaceStack> surface_stack;
    ms::SessionContainer container;
    ms::NullSessionListener session_listener;
    mtd::StubSurfaceFactory stub_surface_factory;
    mtd::StubDisplay display{2};
    mtd::NullEventSink event_sink;
    mtd::StubBufferAllocator allocator;
    mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver> display_config_registrar;

    ms::SessionManager session_manager{mt::fake_shared(surface_stack),
        mt::fake_shared(stub_surface_factory),
        mt::fake_shared(container),
        std::make_shared<mtd::NullSessionEventSink>(),
        mt::fake_shared(session_listener),
        mt::fake_shared(display),
        std::make_shared<mtd::NullANRDetector>(),
        mt::fake_shared(allocator),
        mt::fake_shared(display_config_registrar)};
};

}

namespace
{
struct SessionManagerSessionListenerSetup : public testing::Test
{
    mtd::MockSurfaceStack surface_stack;
    ms::SessionContainer container;
    testing::NiceMock<mtd::MockSessionListener> session_listener;
    mtd::StubSurfaceFactory stub_surface_factory;
    mtd::StubDisplay display{2};
    mtd::StubBufferAllocator allocator;
    mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver> display_config_registrar;

    ms::SessionManager session_manager{
        mt::fake_shared(surface_stack),
        mt::fake_shared(stub_surface_factory),
        mt::fake_shared(container),
        std::make_shared<mtd::NullSessionEventSink>(),
        mt::fake_shared(session_listener),
        mt::fake_shared(display),
        std::make_shared<mtd::NullANRDetector>(),
        mt::fake_shared(allocator),
        mt::fake_shared(display_config_registrar)};
};
}

TEST_F(SessionManagerSessionListenerSetup, session_listener_is_notified_of_lifecycle)
{
    using namespace ::testing;

    EXPECT_CALL(session_listener, starting(_)).Times(1);
    EXPECT_CALL(session_listener, stopping(_)).Times(1);

    auto session = session_manager.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    session_manager.close_session(session);
}

TEST_F(SessionManagerSessionListenerSetup, additional_listeners_receive_session_callbacks)
{
    using namespace ::testing;

    auto additional_listener = std::make_shared<testing::NiceMock<mtd::MockSessionListener>>();
    EXPECT_CALL(*additional_listener, starting(_)).Times(1);
    EXPECT_CALL(*additional_listener, stopping(_)).Times(1);

    session_manager.add_listener(additional_listener);
    auto session = session_manager.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    session_manager.close_session(session);
}

TEST_F(SessionManagerSessionListenerSetup, additional_listeners_receive_focus_changes)
{
    using namespace ::testing;

    auto additional_listener = std::make_shared<testing::NiceMock<mtd::MockSessionListener>>();
    EXPECT_CALL(*additional_listener, starting(_)).Times(1);
    EXPECT_CALL(*additional_listener, focused(_)).Times(1);
    EXPECT_CALL(*additional_listener, unfocused()).Times(1);

    session_manager.add_listener(additional_listener);
    auto session = session_manager.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    session_manager.set_focus_to(session);
    session_manager.unset_focus();
}

TEST_F(SessionManagerSessionListenerSetup, additional_listeners_receive_surface_creation)
{
    using namespace ::testing;
    ms::NullSurfaceObserver observer;
    auto additional_listener = std::make_shared<testing::NiceMock<mtd::MockSessionListener>>();
    EXPECT_CALL(*additional_listener, starting(_)).Times(1);
    EXPECT_CALL(*additional_listener, surface_created(_,_)).Times(1);

    session_manager.add_listener(additional_listener);
    auto session = session_manager.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto bs = std::dynamic_pointer_cast<mc::BufferStream>(session->create_buffer_stream(
        mg::BufferProperties{{640, 480}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware}));
    session->create_surface(nullptr, {}, mt::make_surface_spec(bs), mt::fake_shared(observer), nullptr);
}

namespace
{
struct SessionManagerSessionEventsSetup : public testing::Test
{
    mtd::MockSurfaceStack surface_stack;
    ms::SessionContainer container;
    MockSessionEventSink session_event_sink;
    testing::NiceMock<mtd::MockSessionListener> session_listener;
    mtd::StubSurfaceFactory stub_surface_factory;
    mtd::StubDisplay display{3};
    mtd::StubBufferAllocator allocator;
    mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver> display_config_registrar;

    ms::SessionManager session_manager{
        mt::fake_shared(surface_stack),
        mt::fake_shared(stub_surface_factory),
        mt::fake_shared(container),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(session_listener),
        mt::fake_shared(display),
        std::make_shared<mtd::NullANRDetector>(),
        mt::fake_shared(allocator),
        mt::fake_shared(display_config_registrar)};
};
}

TEST_F(SessionManagerSessionEventsSetup, session_event_sink_is_notified_of_lifecycle)
{
    using namespace ::testing;

    auto session = session_manager.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = session_manager.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());

    Mock::VerifyAndClearExpectations(&session_event_sink);

    EXPECT_CALL(session_event_sink, handle_focus_change(_)).Times(AnyNumber());
    EXPECT_CALL(session_event_sink, handle_no_focus()).Times(AnyNumber());

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);

    session_manager.close_session(session1);
    session_manager.close_session(session);
}
