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

#include "mir/shell/abstract_shell.h"

#include "mir/events/event_builders.h"
#include "mir/scene/session.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/session_container.h"
#include "mir/scene/surface_factory.h"
#include "mir/shell/decoration.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/wayland/weak.h"

#include "src/server/report/null/shell_report.h"
#include "src/include/server/mir/scene/session_event_sink.h"
#include "src/server/scene/session_manager.h"
#include "src/server/shell/decoration/null_manager.h"

#include "mir/test/doubles/mock_window_manager.h"
#include "mir/test/doubles/mock_surface_stack.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/stub_surface.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/null_prompt_session_manager.h"
#include "mir/test/doubles/stub_input_targeter.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/null_application_not_responding_detector.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/stub_observer_registrar.h"

#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"
#include "mir/test/make_surface_spec.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mev = mir::events;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
MATCHER_P(WeakPtrTo, p, "")
{
  return !arg.owner_before(p) && !p.owner_before(arg);
}

struct MockSessionContainer : public ms::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<ms::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<ms::Session> const&));
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<ms::Session>(std::shared_ptr<ms::Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<ms::Session> const&)>));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    ~MockSessionContainer() noexcept {}
};

struct MockSessionEventSink : public ms::SessionEventSink
{
    MOCK_METHOD1(handle_focus_change, void(std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD0(handle_no_focus, void());
    MOCK_METHOD1(handle_session_stopping, void(std::shared_ptr<ms::Session> const& session));
};

struct MockSessionManager : ms::SessionManager
{
    using ms::SessionManager::SessionManager;

    // Must wrap the constructor as the NiceMock<> constructor can only take 10 arguments on older platforms
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
              std::make_shared<mtd::NullANRDetector>(),
              std::make_shared<mtd::StubBufferAllocator>(),
              std::make_shared<mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver>>()}
    {
    }

    MOCK_METHOD1(set_focus_to, void (std::shared_ptr<ms::Session> const& focus));

    void unmocked_set_focus_to(std::shared_ptr<ms::Session> const& focus)
    { ms::SessionManager::set_focus_to(focus); }
};

struct MockSurfaceFactory : public ms::SurfaceFactory
{
    MOCK_METHOD4(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<ms::Session> const&,
        mw::Weak<mf::WlSurface> const&,
        std::list<ms::StreamInfo> const&,
        msh::SurfaceSpecification const&));
};

struct MockDecorationManager : public msh::decoration::NullManager
{
    MOCK_METHOD1(decorate, void(std::shared_ptr<ms::Surface> const&));
    MOCK_METHOD2(
        create_decoration,
        std::unique_ptr<mir::shell::decoration::Decoration>(std::shared_ptr<mir::shell::Shell> const& shell, std::shared_ptr<mir::scene::Surface> const& surface));
};

using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct AbstractShell : Test
{
    NiceMock<mtd::MockSurface> mock_surface;
    NiceMock<mtd::MockSurface> mock_surface1;
    NiceMock<mtd::MockSurface> mock_surface2;
    NiceMock<mtd::MockSurface> mock_surface_child;
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
        std::make_shared<mtd::NullPromptSessionManager>(),
        std::make_shared<mir::report::null::ShellReport>(),
        [this](msh::FocusController*) { return wm = std::make_shared<NiceMockWindowManager>(); },
        mt::fake_shared(seat),
        mt::fake_shared(decoration_manager)};

    void SetUp() override
    {
        ON_CALL(session_manager, set_focus_to(_)).
            WillByDefault(Invoke(&session_manager, &MockSessionManager::unmocked_set_focus_to));
        ON_CALL(mock_surface, size())
            .WillByDefault(Return(geom::Size{}));
        ON_CALL(surface_factory, create_surface(_, _, _, _))
            .WillByDefault(Return(mt::fake_shared(mock_surface)));
        ON_CALL(seat, create_device_state())
            .WillByDefault(Invoke(
                    []()
                    {
                        return mev::make_input_configure_event(
                            0ns, 0, mir_input_event_modifier_none, 0.0f, 0.0f,
                            std::vector<mev::InputDeviceState>());
                    }));
    }

    auto create_surface(ms::Surface& surface) -> std::shared_ptr<ms::Surface>
    {
        auto const session = shell.open_session(
            __LINE__,
            mir::Fd{mir::Fd::invalid},
            "Foo",
            std::shared_ptr<mf::EventSink>());
        EXPECT_CALL(surface_factory, create_surface(_, _, _, _))
            .WillOnce(Return(mt::fake_shared(surface)));
        return shell.create_surface(
            session,
            {},
            mt::make_surface_spec(session->create_buffer_stream(properties)),
            nullptr,
            nullptr);
    }

    std::chrono::nanoseconds const event_timestamp = std::chrono::nanoseconds(0);
    mg::BufferProperties properties { geom::Size{1,1}, mir_pixel_format_abgr_8888, mg::BufferUsage::software};
};
}

TEST_F(AbstractShell, open_session_adds_session_to_window_manager)
{
    std::shared_ptr<ms::Session> new_session;

    InSequence s;
    EXPECT_CALL(*wm, add_session(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    EXPECT_EQ(session, new_session);
}

TEST_F(AbstractShell, close_session_removes_session_from_window_manager)
{
    std::shared_ptr<ms::Session> old_session;

    InSequence s;
    EXPECT_CALL(*wm, remove_session(_)).WillOnce(SaveArg<0>(&old_session));

    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    shell.close_session(session);
    EXPECT_EQ(session, old_session);
}

TEST_F(AbstractShell, close_session_notifies_session_event_sink)
{
    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_session_stopping(session1));
    EXPECT_CALL(session_event_sink, handle_session_stopping(session));

    shell.close_session(session1);
    shell.close_session(session);
}

TEST_F(AbstractShell, close_session_removes_existing_session_surfaces_from_window_manager)
{
    mtd::StubSurface surface1;
    mtd::StubSurface surface2;
    mtd::StubSurface surface3;
    EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).
        WillOnce(Return(mt::fake_shared(surface1))).
        WillOnce(Return(mt::fake_shared(surface2))).
        WillOnce(Return(mt::fake_shared(surface3)));

    auto const session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const created_surface1 = shell.create_surface(session, {},
        mt::make_surface_spec(session->create_buffer_stream(properties)), nullptr, nullptr);
    auto const created_surface2 = shell.create_surface(session, {},
        mt::make_surface_spec(session->create_buffer_stream(properties)), nullptr, nullptr);
    auto const created_surface3 = shell.create_surface(session, {},
        mt::make_surface_spec(session->create_buffer_stream(properties)), nullptr, nullptr);

    session->destroy_surface(created_surface2);

    Expectation remove1 = EXPECT_CALL(
        *wm, remove_surface(session, WeakPtrTo(created_surface1)));
    Expectation remove3 = EXPECT_CALL(
        *wm, remove_surface(session, WeakPtrTo(created_surface3)));
    EXPECT_CALL(*wm, remove_session(session)).After(remove1, remove3);

    shell.close_session(session);
}

TEST_F(AbstractShell, create_surface_provides_create_parameters_to_window_manager)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = mt::make_surface_spec(session->create_buffer_stream(properties));
    EXPECT_CALL(*wm, add_surface(session, params, _));

    shell.create_surface(session, {}, params, nullptr, nullptr);
}

TEST_F(AbstractShell, create_surface_allows_window_manager_to_set_create_parameters)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = mt::make_surface_spec(session->create_buffer_stream(properties));
    params.width = geom::Width{100};
    params.input_mode = mi::InputReceptionMode::receives_all_input;

    EXPECT_CALL(surface_stack, add_surface(_, mi::InputReceptionMode::receives_all_input));

    EXPECT_CALL(*wm, add_surface(session, params, _)).WillOnce(Invoke(
        [&](std::shared_ptr<ms::Session> const& session,
            msh::SurfaceSpecification const&,
            std::function<std::shared_ptr<ms::Surface>(std::shared_ptr<ms::Session> const& session, msh::SurfaceSpecification const&)> const& build)
            { return build(session, params); }));

    shell.create_surface(session, {}, params, nullptr, nullptr);
}

TEST_F(AbstractShell, create_surface_allows_window_manager_to_enable_ssd)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = mt::make_surface_spec(session->create_buffer_stream(properties));

    EXPECT_CALL(decoration_manager, decorate(_))
        .Times(1);

    EXPECT_CALL(*wm, add_surface(session, params, _)).WillOnce(Invoke(
        [&](std::shared_ptr<ms::Session> const& session,
            msh::SurfaceSpecification const& params,
            std::function<std::shared_ptr<ms::Surface>(
                std::shared_ptr<ms::Session> const& session,
                msh::SurfaceSpecification const&)> const& build)
            {
                auto modified_params = params;
                modified_params.server_side_decorated = true;
                return build(session, modified_params);
            }));

    shell.create_surface(session, {}, params, nullptr, nullptr);
}

TEST_F(AbstractShell, create_surface_allows_window_manager_to_disable_ssd)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = mt::make_surface_spec(session->create_buffer_stream(properties));
    params.server_side_decorated = true;

    EXPECT_CALL(decoration_manager, decorate(_))
        .Times(0);

    EXPECT_CALL(*wm, add_surface(session, params, _)).WillOnce(Invoke(
        [&](std::shared_ptr<ms::Session> const& session,
            msh::SurfaceSpecification const& params,
            std::function<std::shared_ptr<ms::Surface>(
                std::shared_ptr<ms::Session> const& session,
                msh::SurfaceSpecification const&)> const& build)
            {
                auto modified_params = params;
                modified_params.server_side_decorated = false;
                return build(session, modified_params);
            }));

    shell.create_surface(session, {}, params, nullptr, nullptr);
}

TEST_F(AbstractShell, create_surface_sets_surfaces_session)
{
    auto const mock_surface = std::make_shared<mtd::MockSurface>();

    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    EXPECT_CALL(surface_factory, create_surface(session, _, _, _)).
        WillOnce(Return(mock_surface));

    auto params = mt::make_surface_spec(session->create_buffer_stream(properties));

    shell.create_surface(session, {}, params, nullptr, nullptr);
}

TEST_F(AbstractShell, destroy_surface_removes_surface_from_window_manager)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));

    auto const surface = shell.create_surface(session, {}, params, nullptr, nullptr);

    EXPECT_CALL(*wm, remove_surface(session, WeakPtrTo(surface)));

    shell.destroy_surface(session, surface);
}

TEST_F(AbstractShell, add_display_adds_display_to_window_manager)
{
    geom::Rectangle const arbitrary_area{{0,0}, {__LINE__,__LINE__}};

    EXPECT_CALL(*wm, add_display(arbitrary_area));

    shell.add_display(arbitrary_area);
}

TEST_F(AbstractShell, remove_display_adds_display_to_window_manager)
{
    geom::Rectangle const arbitrary_area{{0,0}, {__LINE__,__LINE__}};

    EXPECT_CALL(*wm, remove_display(arbitrary_area));

    shell.remove_display(arbitrary_area);
}

TEST_F(AbstractShell, key_input_events_are_handled_by_window_manager)
{
    MirKeyboardAction const action{mir_keyboard_action_down};
    xkb_keysym_t const keysym{0};
    int const scan_code{0};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_none};
    auto const event = mir::events::make_key_event(
        mir_input_event_type_key,
        event_timestamp,
        action,
        keysym,
        scan_code,
        modifiers);

    EXPECT_CALL(*wm, handle_keyboard_event(_))
        .WillOnce(Return(false))
        .WillOnce(Return(true));

    EXPECT_FALSE(shell.handle(*event));
    EXPECT_TRUE(shell.handle(*event));
}

TEST_F(AbstractShell, touch_input_events_are_handled_by_window_manager)
{
    MirInputEventModifiers const modifiers{mir_input_event_modifier_none};
    auto const event = mir::events::make_touch_event(
        mir_input_event_type_touch,
        event_timestamp,
        modifiers);

    EXPECT_CALL(*wm, handle_touch_event(_))
        .WillOnce(Return(false))
        .WillOnce(Return(true));

    EXPECT_FALSE(shell.handle(*event));
    EXPECT_TRUE(shell.handle(*event));
}

TEST_F(AbstractShell, pointer_input_events_are_handled_by_window_manager)
{
    MirInputEventModifiers const modifiers{mir_input_event_modifier_none};
    MirPointerAction const action{mir_pointer_action_button_down};
    auto const buttons_pressed = mir_pointer_button_primary;
    float const x_axis_value{0.0};
    float const y_axis_value{0.0};
    float const hscroll_value{0.0};
    float const vscroll_value{0.0};
    float const relative_x_value{0.0};
    float const relative_y_value{0.0};
    auto const event = mir::events::make_pointer_event(
        mir_input_event_type_pointer,
        event_timestamp,
        modifiers,
        action,
        buttons_pressed,
        x_axis_value,
        y_axis_value,
        hscroll_value,
        vscroll_value,
        relative_x_value,
        relative_y_value);

    EXPECT_CALL(*wm, handle_pointer_event(_))
        .WillOnce(Return(false))
        .WillOnce(Return(true));

    EXPECT_FALSE(shell.handle(*event));
    EXPECT_TRUE(shell.handle(*event));
}

TEST_F(AbstractShell, as_focus_controller_set_focus_to_notifies_session_event_sink)
{
    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_focus_change(session1));
    EXPECT_CALL(session_event_sink, handle_focus_change(session));
    EXPECT_CALL(session_event_sink, handle_no_focus());

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(session1, {});
    focus_controller.set_focus_to(session, {});
    focus_controller.set_focus_to({}, {});
}

TEST_F(AbstractShell, as_focus_controller_focus_next_session_notifies_session_event_sink)
{
    msh::FocusController& focus_controller = shell;
    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto const params2 = mt::make_surface_spec(session->create_buffer_stream(properties));
    shell.create_surface(session, {}, params, nullptr, nullptr);
    shell.create_surface(session1, {}, params2, nullptr, nullptr);

    focus_controller.set_focus_to(session, {});

    EXPECT_CALL(session_event_sink, handle_focus_change(session1));

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell, as_focus_controller_focused_session_follows_focus)
{
    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto const params2 = mt::make_surface_spec(session->create_buffer_stream(properties));
    shell.create_surface(session, {}, params, nullptr, nullptr);
    shell.create_surface(session1, {}, params2, nullptr, nullptr);

    msh::FocusController& focus_controller = shell;

    focus_controller.set_focus_to(session, {});
    EXPECT_THAT(focus_controller.focused_session(), Eq(session));
    focus_controller.set_focus_to(session1, {});
    EXPECT_THAT(focus_controller.focused_session(), Eq(session1));
    focus_controller.focus_next_session();
    EXPECT_THAT(focus_controller.focused_session(), Eq(session));
}

TEST_F(AbstractShell, as_focus_controller_focused_surface_follows_focus)
{
    auto const session0 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());
    ON_CALL(mock_surface1, size()).WillByDefault(Return(geom::Size{}));
    EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(AnyNumber())
        .WillOnce(Return(mt::fake_shared(mock_surface1)))
        .WillOnce(Return(mt::fake_shared(mock_surface)));

    auto const params0 = mt::make_surface_spec(session0->create_buffer_stream(properties));
    auto const params1 = mt::make_surface_spec(session1->create_buffer_stream(properties));
    auto const surface0 = shell.create_surface(session0, {}, params0, nullptr, nullptr);
    auto const surface1 = shell.create_surface(session1, {}, params1, nullptr, nullptr);

    msh::FocusController& focus_controller = shell;

    focus_controller.set_focus_to(session0, surface0);
    EXPECT_THAT(focus_controller.focused_surface(), Eq(surface0));
    focus_controller.set_focus_to(session1, surface1);
    EXPECT_THAT(focus_controller.focused_surface(), Eq(surface1));
    focus_controller.focus_next_session();
    EXPECT_THAT(focus_controller.focused_surface(), Eq(surface0));

    shell.destroy_surface(session0, surface0);
    shell.destroy_surface(session1, surface1);
    shell.close_session(session1);
    shell.close_session(session0);
}

TEST_F(AbstractShell, as_focus_controller_delegates_surface_at_to_surface_stack)
{
    auto const surface = mt::fake_shared(mock_surface);
    geom::Point const cursor{__LINE__, __LINE__};

    EXPECT_CALL(surface_stack, surface_at(cursor)).
        WillOnce(Return(surface));

    msh::FocusController& focus_controller = shell;

    EXPECT_THAT(focus_controller.surface_at(cursor), Eq(surface));
}

TEST_F(AbstractShell, as_focus_controller_focus_next_session_skips_surfaceless_sessions)
{
    msh::FocusController& focus_controller = shell;
    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Surfaceless", std::shared_ptr<mf::EventSink>());
    auto session2 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto created_surface = shell.create_surface(session, {}, params, nullptr, nullptr);
    auto const params2 = mt::make_surface_spec(session2->create_buffer_stream(properties));
    shell.create_surface(session2, {}, params2, nullptr, nullptr);

    focus_controller.set_focus_to(session, created_surface);

    EXPECT_CALL(session_event_sink, handle_focus_change(session2));

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell,
       as_focus_controller_focus_next_session_does_not_focus_any_session_if_no_session_has_surfaces)
{
    msh::FocusController& focus_controller = shell;
    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Surfaceless", std::shared_ptr<mf::EventSink>());
    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);

    focus_controller.set_focus_to(session, surface);

    session->destroy_surface(surface);

    EXPECT_CALL(session_event_sink, handle_no_focus());

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell, modify_surface_with_only_streams_doesnt_call_into_wm)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);

    msh::SurfaceSpecification stream_modification;
    stream_modification.streams = std::vector<msh::StreamSpecification>{};

    EXPECT_CALL(*wm, modify_surface(_,_,_)).Times(0);

    shell.modify_surface(session, surface, stream_modification);
}

TEST_F(AbstractShell, modify_surface_does_not_call_wm_for_empty_changes)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));

    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);

    msh::SurfaceSpecification stream_modification;

    EXPECT_CALL(*wm, modify_surface(_,_,_)).Times(0);

    shell.modify_surface(session, surface, stream_modification);
}

TEST_F(AbstractShell, size_gets_adjusted_for_windows_with_margins)
{
    geom::DeltaY const top{3};
    geom::DeltaX const left{4};
    geom::DeltaY const bottom{2};
    geom::DeltaX const right{6};
    geom::Size const content_size{102, 87};
    geom::Size const window_size{
        content_size.width + left + right,
        content_size.height + top + bottom};
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);
    surface->resize({50, 50});
    surface->set_window_margins(top, left, bottom, right);

    msh::SurfaceSpecification modifications;
    modifications.width = content_size.width;
    modifications.height = content_size.height;

    msh::SurfaceSpecification wm_modifications;
    EXPECT_CALL(*wm, modify_surface(_,_,_)).WillOnce(SaveArg<2>(&wm_modifications));

    shell.modify_surface(session, surface, modifications);

    ASSERT_THAT(wm_modifications.width, Eq(mir::optional_value<geom::Width>(window_size.width)));
    ASSERT_THAT(wm_modifications.height, Eq(mir::optional_value<geom::Height>(window_size.height)));
}

TEST_F(AbstractShell, max_size_gets_adjusted_for_windows_with_margins)
{
    geom::DeltaY const top{3};
    geom::DeltaX const left{4};
    geom::DeltaY const bottom{2};
    geom::DeltaX const right{6};
    geom::Size const content_size{102, 87};
    geom::Size const window_size{
        content_size.width + left + right,
        content_size.height + top + bottom};
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);
    surface->resize({50, 50});
    surface->set_window_margins(top, left, bottom, right);

    msh::SurfaceSpecification modifications;
    modifications.max_width = content_size.width;
    modifications.max_height = content_size.height;

    msh::SurfaceSpecification wm_modifications;
    EXPECT_CALL(*wm, modify_surface(_,_,_)).WillOnce(SaveArg<2>(&wm_modifications));

    shell.modify_surface(session, surface, modifications);

    ASSERT_THAT(wm_modifications.max_width, Eq(mir::optional_value<geom::Width>(window_size.width)));
    ASSERT_THAT(wm_modifications.max_height, Eq(mir::optional_value<geom::Height>(window_size.height)));
}

TEST_F(AbstractShell, min_size_gets_adjusted_for_windows_with_margins)
{
    geom::DeltaY const top{3};
    geom::DeltaX const left{4};
    geom::DeltaY const bottom{2};
    geom::DeltaX const right{6};
    geom::Size const content_size{102, 87};
    geom::Size const window_size{
        content_size.width + left + right,
        content_size.height + top + bottom};
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);
    surface->resize({50, 50});
    surface->set_window_margins(top, left, bottom, right);

    msh::SurfaceSpecification modifications;
    modifications.min_width = content_size.width;
    modifications.min_height = content_size.height;

    msh::SurfaceSpecification wm_modifications;
    EXPECT_CALL(*wm, modify_surface(_,_,_)).WillOnce(SaveArg<2>(&wm_modifications));

    shell.modify_surface(session, surface, modifications);

    ASSERT_THAT(wm_modifications.min_width, Eq(mir::optional_value<geom::Width>(window_size.width)));
    ASSERT_THAT(wm_modifications.min_height, Eq(mir::optional_value<geom::Height>(window_size.height)));
}

TEST_F(AbstractShell, aux_rect_gets_adjusted_for_windows_with_margins)
{
    geom::DeltaY const top{3};
    geom::DeltaX const left{4};
    geom::DeltaY const bottom{2};
    geom::DeltaX const right{6};
    geom::Size const content_size{102, 87};
    geom::Rectangle const frontend_aux_rect{{23, 20}, {16, 18}};
    geom::Rectangle const adjusted_aux_rect{
        frontend_aux_rect.top_left + geom::Displacement{left, top},
        frontend_aux_rect.size};
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);
    surface->resize(content_size);
    surface->set_window_margins(top, left, bottom, right);

    msh::SurfaceSpecification modifications;
    modifications.aux_rect = frontend_aux_rect;

    msh::SurfaceSpecification wm_modifications;
    EXPECT_CALL(*wm, modify_surface(_,_,_)).WillOnce(SaveArg<2>(&wm_modifications));

    shell.modify_surface(session, surface, modifications);

    ASSERT_THAT(wm_modifications.aux_rect, Eq(mir::optional_value<geom::Rectangle>(adjusted_aux_rect)));
}

// lp:1625401
TEST_F(AbstractShell, when_remaining_session_has_no_surface_focus_next_session_doesnt_loop_endlessly)
{
    std::shared_ptr<ms::Session> empty_session =
        shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "empty_session", std::shared_ptr<mf::EventSink>());

    {
        std::shared_ptr<ms::Session> another_session =
            shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "another_session", std::shared_ptr<mf::EventSink>());

        auto creation_params = mt::make_surface_spec(another_session->create_buffer_stream(properties));
        auto surface = shell.create_surface(another_session, {}, creation_params, nullptr, nullptr);

        shell.set_focus_to(another_session, surface);

        shell.close_session(another_session);
    }

    shell.focus_next_session();
}

TEST_F(AbstractShell, focus_can_be_set)
{
    EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(AnyNumber())
        .WillOnce(Return(mt::fake_shared(mock_surface1)))
        .WillOnce(Return(mt::fake_shared(mock_surface)));

    msh::FocusController& focus_controller = shell;
    auto const session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto const created_surface1 = shell.create_surface(session, {}, params, nullptr, nullptr);
    auto const created_surface2 = shell.create_surface(session, {}, params, nullptr, nullptr);

    focus_controller.set_focus_to(session, created_surface2);
    EXPECT_THAT(created_surface1->focus_state(), Eq(mir_window_focus_state_unfocused));
    EXPECT_THAT(created_surface2->focus_state(), Eq(mir_window_focus_state_focused));

    focus_controller.set_focus_to(session, created_surface1);
    EXPECT_THAT(created_surface1->focus_state(), Eq(mir_window_focus_state_focused));
    EXPECT_THAT(created_surface2->focus_state(), Eq(mir_window_focus_state_unfocused));
}

TEST_F(AbstractShell, setting_focus_to_child_makes_parent_active)
{
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(mt::fake_shared(mock_surface)));
    EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(AnyNumber())
        .WillOnce(Return(mt::fake_shared(mock_surface)))
        .WillOnce(Return(mt::fake_shared(mock_surface1)))
        .WillOnce(Return(mt::fake_shared(mock_surface2)));

    msh::FocusController& focus_controller = shell;
    auto const session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto const parent_surface = shell.create_surface(session, {}, params, nullptr, nullptr);
    auto const child_surface = shell.create_surface(session, {}, params, nullptr, nullptr);
    auto const other_surface = shell.create_surface(session, {}, params, nullptr, nullptr);

    ASSERT_THAT(child_surface->parent(), Eq(parent_surface));

    focus_controller.set_focus_to(session, other_surface);
    EXPECT_THAT(child_surface->focus_state(), Eq(mir_window_focus_state_unfocused));
    EXPECT_THAT(parent_surface->focus_state(), Eq(mir_window_focus_state_unfocused));

    focus_controller.set_focus_to(session, child_surface);
    EXPECT_THAT(child_surface->focus_state(), Eq(mir_window_focus_state_focused));
    EXPECT_THAT(parent_surface->focus_state(), Eq(mir_window_focus_state_active));
}

TEST_F(AbstractShell, does_not_give_keyboard_focus_to_menu)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_child = create_surface(mock_surface_child);
    ON_CALL(mock_surface_child, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface_child, type())
        .WillByDefault(Return(mir_window_type_menu));

    msh::FocusController& focus_controller = shell;

    EXPECT_CALL(mock_surface_child, set_focus_state(mir_window_focus_state_active));
    EXPECT_CALL(mock_surface, set_focus_state(mir_window_focus_state_focused));
    focus_controller.set_focus_to(surface_child->session().lock(), surface_child);
}

TEST_F(AbstractShell, does_not_give_keyboard_focus_to_tip)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_child = create_surface(mock_surface_child);
    ON_CALL(mock_surface_child, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface_child, type())
        .WillByDefault(Return(mir_window_type_tip));

    msh::FocusController& focus_controller = shell;

    EXPECT_CALL(mock_surface_child, set_focus_state(mir_window_focus_state_active));
    EXPECT_CALL(mock_surface, set_focus_state(mir_window_focus_state_focused));
    focus_controller.set_focus_to(surface_child->session().lock(), surface_child);
}

TEST_F(AbstractShell, does_not_deactivate_parent_when_switching_children)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_child_a = create_surface(mock_surface1);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));

    auto const surface_child_b = create_surface(mock_surface2);
    ON_CALL(mock_surface2, parent())
        .WillByDefault(Return(surface_parent));

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(surface_child_a->session().lock(), surface_child_a);

    EXPECT_CALL(mock_surface, set_focus_state(mir_window_focus_state_active)).Times(AnyNumber());
    focus_controller.set_focus_to(surface_child_b->session().lock(), surface_child_b);
}

TEST_F(AbstractShell, makes_parent_active_when_switching_to_child)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_child = create_surface(mock_surface_child);
    ON_CALL(mock_surface_child, parent())
        .WillByDefault(Return(surface_parent));

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(surface_parent->session().lock(), surface_parent);
    /* window         | expected focus state
     * -----------------------------------------------
     * surface_parent | mir_window_focus_state_focused
     * surface_child  | <don't care>
    */

    EXPECT_CALL(mock_surface, set_focus_state(mir_window_focus_state_active));
    focus_controller.set_focus_to(surface_child->session().lock(), surface_child);
    /* window         | expected focus state
     * -----------------------------------------------
     * surface_parent | mir_window_focus_state_active
     * surface_child  | mir_window_focus_state_focused
    */

}

TEST_F(AbstractShell, makes_parent_focused_when_switching_back_from_child)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_child = create_surface(mock_surface_child);
    ON_CALL(mock_surface_child, parent())
        .WillByDefault(Return(surface_parent));

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(surface_child->session().lock(), surface_child);
    /* window         | expected focus state
     * -----------------------------------------------
     * surface_parent | mir_window_focus_state_active
     * surface_child  | mir_window_focus_state_focused
    */

    EXPECT_CALL(mock_surface, set_focus_state(mir_window_focus_state_focused));
    focus_controller.set_focus_to(surface_parent->session().lock(), surface_parent);
    /* window         | expected focus state
     * -----------------------------------------------
     * surface_parent | mir_window_focus_state_focused
     * surface_child  | <don't care>
    */
}

TEST_F(AbstractShell, menu_can_be_made_within_popup_grab_tree)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_menu = create_surface(mock_surface1);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface1, type())
        .WillByDefault(Return(mir_window_type_menu));

    msh::FocusController& focus_controller = shell;
    focus_controller.set_popup_grab_tree(surface_parent);

    EXPECT_CALL(mock_surface1, request_client_surface_close()).Times(0);

    shell.surface_ready(surface_menu);
}

TEST_F(AbstractShell, menu_closed_when_made_outside_of_popup_grab_tree)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_menu = create_surface(mock_surface1);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface1, type())
        .WillByDefault(Return(mir_window_type_menu));

    auto const other_surface = create_surface(mock_surface2);

    msh::FocusController& focus_controller = shell;
    focus_controller.set_popup_grab_tree(other_surface);

    EXPECT_CALL(mock_surface1, request_client_surface_close());

    shell.surface_ready(surface_menu);
}

TEST_F(AbstractShell, menu_closed_when_popup_grab_tree_changes)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_menu = create_surface(mock_surface1);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface1, type())
        .WillByDefault(Return(mir_window_type_menu));

    auto const other_surface = create_surface(mock_surface2);

    msh::FocusController& focus_controller = shell;
    focus_controller.set_popup_grab_tree(surface_parent);
    shell.surface_ready(surface_menu);

    EXPECT_CALL(mock_surface1, request_client_surface_close());

    focus_controller.set_popup_grab_tree(other_surface);
}

TEST_F(AbstractShell, popup_grab_tree_not_set_based_on_focus)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_menu = create_surface(mock_surface1);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface1, type())
        .WillByDefault(Return(mir_window_type_menu));

    auto const other_surface = create_surface(mock_surface2);

    msh::FocusController& focus_controller = shell;
    focus_controller.set_popup_grab_tree(surface_parent);
    shell.surface_ready(surface_menu);

    EXPECT_CALL(mock_surface1, request_client_surface_close()).Times(0);

    focus_controller.set_focus_to(other_surface->session().lock(), other_surface);
}

TEST_F(AbstractShell, popup_grab_tree_can_be_set_based_on_child)
{
    auto const surface_parent = create_surface(mock_surface);

    auto const surface_menu = create_surface(mock_surface1);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface1, type())
        .WillByDefault(Return(mir_window_type_menu));

    auto const surface_tip = create_surface(mock_surface2);
    ON_CALL(mock_surface1, parent())
        .WillByDefault(Return(surface_parent));
    ON_CALL(mock_surface1, type())
        .WillByDefault(Return(mir_window_type_tip));

    msh::FocusController& focus_controller = shell;
    shell.surface_ready(surface_tip);
    shell.surface_ready(surface_menu);
    focus_controller.set_popup_grab_tree(surface_tip);

    EXPECT_CALL(mock_surface1, request_client_surface_close()).Times(0);

    focus_controller.set_popup_grab_tree(surface_tip);
}

// Regression test for https://github.com/MirServer/mir/issues/2279
TEST_F(AbstractShell, focus_next_session_allows_later_focusing_same_window)
{
    EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(AnyNumber())
        .WillOnce(Return(mt::fake_shared(mock_surface1)))
        .WillOnce(Return(mt::fake_shared(mock_surface)));

    msh::FocusController& focus_controller = shell;
    auto session1 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session2 = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params1 = mt::make_surface_spec(session1->create_buffer_stream(properties));
    auto created_surface1 = shell.create_surface(session1, {}, params1, nullptr, nullptr);
    auto const params2 = mt::make_surface_spec(session2->create_buffer_stream(properties));
    auto created_surface2 = shell.create_surface(session2, {}, params2, nullptr, nullptr);

    focus_controller.set_focus_to(session1, created_surface1);
    focus_controller.focus_next_session();
    focus_controller.set_focus_to(session2, created_surface2);

    EXPECT_THAT(created_surface1->focus_state(), Eq(mir_window_focus_state_unfocused));
    EXPECT_THAT(created_surface2->focus_state(), Eq(mir_window_focus_state_focused));
}

namespace mir
{
namespace scene
{
// The next test is easier to write if we can compare std::weak_ptr<ms::Surface>
inline bool operator==(
    std::weak_ptr<ms::Surface> const& lhs,
    std::weak_ptr<ms::Surface> const& rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}
}
}

TEST_F(AbstractShell, as_focus_controller_delegates_raise_to_surface_stack)
{
    msh::SurfaceSet const surfaces{mt::fake_shared(mock_surface)};

    EXPECT_CALL(surface_stack, raise(surfaces));

    msh::FocusController& focus_controller = shell;

    focus_controller.raise(surfaces);
}

TEST_F(AbstractShell, as_focus_controller_emits_input_device_state_event_on_focus_change)
{
    EXPECT_CALL(mock_surface, consume(mt::InputDeviceStateEvent())).Times(1);

    auto session = shell.open_session(__LINE__, mir::Fd{mir::Fd::invalid}, "some", std::shared_ptr<mf::EventSink>());
    auto creation_params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, {}, creation_params, nullptr, nullptr);

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(session, surface);
}
