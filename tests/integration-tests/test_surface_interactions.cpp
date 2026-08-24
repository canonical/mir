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

#include <mir/shell/abstract_shell.h>

#include <mir/events/event_builders.h>
#include <mir/scene/session.h>
#include <mir/scene/null_session_listener.h>
#include <mir/scene/session_container.h>
#include <mir/scene/surface_factory.h>
#include <mir/scene/basic_surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/wayland/weak.h>

#include "src/server/report/null/shell_report.h"
#include "src/server/report/null_report_factory.h"
#include "src/include/server/mir/scene/session_event_sink.h"
#include "src/server/scene/session_manager.h"
#include "src/server/shell/decoration/null_manager.h"

#include <mir/test/doubles/mock_window_manager.h>
#include <mir/test/doubles/mock_surface_stack.h>
#include <mir/test/doubles/mock_buffer_stream.h>
#include <mir/test/doubles/null_event_sink.h>
#include <mir/test/doubles/stub_input_targeter.h>
#include <mir/test/doubles/stub_buffer_allocator.h>
#include <mir/test/doubles/stub_display.h>
#include <mir/test/doubles/mock_input_seat.h>
#include <mir/test/doubles/stub_observer_registrar.h>
#include <mir/test/doubles/stub_cursor_image.h>

#include <mir/test/fake_shared.h>
#include <mir/test/make_surface_spec.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mev = mir::events;
namespace mr = mir::report;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
struct MockSessionContainer : public ms::SessionContainer
{
    MOCK_METHOD(void, insert_session, (std::shared_ptr<ms::Session> const&));
    MOCK_METHOD(void, remove_session, (std::shared_ptr<ms::Session> const&));
    MOCK_METHOD(std::shared_ptr<ms::Session>, successor_of, (std::shared_ptr<ms::Session> const&), (const));
    MOCK_METHOD(void, for_each, (std::function<void(std::shared_ptr<ms::Session> const&)>), (const));
    MOCK_METHOD(void, lock, ());
    MOCK_METHOD(void, unlock, ());
    ~MockSessionContainer() noexcept {}
};

struct MockSessionEventSink : public ms::SessionEventSink
{
    MOCK_METHOD(void, handle_focus_change, (std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD(void, handle_no_focus, ());
    MOCK_METHOD(void, handle_session_stopping, (std::shared_ptr<ms::Session> const& session));
};

struct MockSessionManager : ms::SessionManager
{
    using ms::SessionManager::SessionManager;

    MockSessionManager(
        std::shared_ptr<msh::SurfaceStack> const& surface_stack,
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory,
        std::shared_ptr<ms::SessionContainer> const& app_container,
        std::shared_ptr<ms::SessionEventSink> const& session_event_sink,
        std::shared_ptr<mg::Display const> const& display)
        : ms::SessionManager{
              surface_stack,
              surface_factory,
              app_container,
              session_event_sink,
              std::make_shared<ms::NullSessionListener>(),
              display,
              std::make_shared<mtd::StubBufferAllocator>(),
              std::make_shared<mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver>>()}
    {
    }

    MOCK_METHOD(void, set_focus_to, (std::shared_ptr<ms::Session> const& focus));

    void unmocked_set_focus_to(std::shared_ptr<ms::Session> const& focus)
    { ms::SessionManager::set_focus_to(focus); }
};

struct MockSurfaceFactory : public ms::SurfaceFactory
{
    MOCK_METHOD(
        std::shared_ptr<ms::Surface>,
        create_surface,
        (std::shared_ptr<ms::Session> const&,
         std::list<ms::StreamInfo> const&,
         msh::SurfaceSpecification const&));
};

struct MockDecorationManager : public msh::decoration::NullManager
{
    MOCK_METHOD(void, decorate, (std::shared_ptr<ms::Surface> const&));
    MOCK_METHOD(
        geom::Size, compute_size_with_decorations, (geom::Size content_size, MirWindowType type, MirWindowState state));
};

using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct SurfaceInteractions : Test
{
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    ms::SessionContainer session_container;
    NiceMock<MockSessionEventSink> session_event_sink;
    NiceMock<MockSurfaceFactory> surface_factory;
    NiceMock<mtd::MockInputSeat> seat;
    NiceMock<MockDecorationManager> decoration_manager;
    mtd::StubDisplay display{3};

    NiceMock<MockSessionManager> session_manager{
        mt::fake_shared(surface_stack),
        mt::fake_shared(surface_factory),
        mt::fake_shared(session_container),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(display)};

    mtd::StubInputTargeter input_targeter;
    std::shared_ptr<NiceMockWindowManager> wm;

    msh::AbstractShell shell{
        mt::fake_shared(input_targeter),
        mt::fake_shared(surface_stack),
        mt::fake_shared(session_manager),
        std::make_shared<mir::report::null::ShellReport>(),
        [this](msh::FocusController*) { return wm = std::make_shared<NiceMockWindowManager>(); },
        mt::fake_shared(seat),
        mt::fake_shared(decoration_manager)};

    std::shared_ptr<mg::CursorImage> cursor_image = std::make_shared<mtd::StubCursorImage>();
    std::shared_ptr<ms::SceneReport> scene_report = mr::null_scene_report();
    std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> display_config_registrar =
        std::make_shared<mtd::StubObserverRegistrar<mg::DisplayConfigurationObserver>>();

    void SetUp() override
    {
        ON_CALL(session_manager, set_focus_to(_))
            .WillByDefault(Invoke(&session_manager, &MockSessionManager::unmocked_set_focus_to));
        ON_CALL(seat, create_device_state())
            .WillByDefault(
                []()
                {
                    return mev::make_input_configure_event(
                        0ns, 0, mir_input_event_modifier_none, 0.0f, 0.0f,
                        std::vector<mev::InputDeviceState>());
                });
        ON_CALL(surface_factory, create_surface(_, _, _))
            .WillByDefault(
                [this](
                    std::shared_ptr<ms::Session> const& session,
                    std::list<ms::StreamInfo> const& streams,
                    msh::SurfaceSpecification const& spec)
                {
                    return std::make_shared<ms::BasicSurface>(
                        session,
                        spec.name.value_or(""),
                        geom::Rectangle{
                            spec.top_left.value_or(geom::Point{}),
                            geom::Size{
                                spec.width.value_or(geom::Width{100}),
                                spec.height.value_or(geom::Height{100})}},
                        spec.parent.value_or(std::weak_ptr<ms::Surface>{}),
                        spec.confine_pointer.value_or(mir_pointer_unconfined),
                        streams,
                        cursor_image,
                        scene_report,
                        display_config_registrar);
                });
    }
};

struct TestSurfaceObserver : ms::NullSurfaceObserver
{
    std::function<void(ms::Surface const*, geom::Point const&)> on_moved;

    void moved_to(ms::Surface const* surf, geom::Point const& top_left) override
    {
        if (on_moved)
            on_moved(surf, top_left);
    }
};

TEST_F(SurfaceInteractions, on_moved_observer_sees_up_to_date_input_state)
{
    auto const session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "TestApp");
    auto mock_stream = std::make_shared<NiceMock<mtd::MockBufferStream>>();
    auto const creation_params = mt::make_surface_spec(mock_stream);

    auto surface = shell.create_surface(session, creation_params, nullptr, nullptr);

    auto observer = std::make_shared<TestSurfaceObserver>();

    geom::Point const new_position{200, 200};
    bool observer_called = false;

    observer->on_moved = [&](ms::Surface const* surf, geom::Point const& top_left)
        {
            observer_called = true;
            EXPECT_THAT(top_left, Eq(new_position));
            EXPECT_THAT(surf->top_left(), Eq(new_position));
            // The surface's default input area (bounding box) should already reflect the new position
            EXPECT_TRUE(surf->input_area_contains({210, 210}));
            EXPECT_FALSE(surf->input_area_contains({5, 5}));
        };

    surface->register_interest(observer);

    EXPECT_CALL(*wm, modify_surface(_, _, _))
        .WillOnce(
            [&](auto, auto surf, auto const& mods)
            {
                if (mods.top_left)
                    surf->move_to(*mods.top_left);
            });

    msh::SurfaceSpecification modifications;
    modifications.top_left = new_position;
    shell.modify_surface(session, surface, modifications);

    EXPECT_TRUE(observer_called);
}
}
