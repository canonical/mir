/*
 * Copyright © 2015-2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan.griffiths@canonical.com>
 */

#include "mir/shell/abstract_shell.h"

#include "mir/events/event_builders.h"
#include "mir/scene/session.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/session_container.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface_factory.h"
#include "mir/graphics/display_configuration_observer.h"

#include "src/server/report/null/shell_report.h"
#include "src/include/server/mir/scene/session_event_sink.h"
#include "src/server/scene/session_manager.h"

#include "mir/test/doubles/mock_window_manager.h"
#include "mir/test/doubles/mock_surface_stack.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/stub_surface.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/null_snapshot_strategy.h"
#include "mir/test/doubles/null_prompt_session_manager.h"
#include "mir/test/doubles/stub_input_targeter.h"
#include "mir/test/doubles/stub_buffer_stream_factory.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/null_application_not_responding_detector.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/stub_observer_registrar.h"

#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
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
              std::make_shared<mtd::StubBufferStreamFactory>(),
              app_container,
              std::make_shared<mtd::NullSnapshotStrategy>(),
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
    MOCK_METHOD3(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<ms::Session> const&,
        std::list<ms::StreamInfo> const&,
        ms::SurfaceCreationParameters const&));
};

using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct AbstractShell : Test
{
    NiceMock<mtd::MockSurface> mock_surface;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    ms::SessionContainer session_container;
    NiceMock<MockSessionEventSink> session_event_sink;
    NiceMock<MockSurfaceFactory> surface_factory;
    NiceMock<mtd::MockInputSeat> seat;
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
        mt::fake_shared(seat)};

    void SetUp() override
    {
        ON_CALL(session_manager, set_focus_to(_)).
            WillByDefault(Invoke(&session_manager, &MockSessionManager::unmocked_set_focus_to));
        ON_CALL(mock_surface, size())
            .WillByDefault(Return(geom::Size{}));
        ON_CALL(surface_factory, create_surface(_, _, _))
            .WillByDefault(Return(mt::fake_shared(mock_surface)));
        ON_CALL(seat, create_device_state())
            .WillByDefault(Invoke(
                    []()
                    {
                        return mev::make_event(0ns, 0, mir_input_event_modifier_none, 0.0f, 0.0f,
                                               std::vector<mev::InputDeviceState>());
                    }));
    }

    std::chrono::nanoseconds const event_timestamp = std::chrono::nanoseconds(0);
    std::vector<uint8_t> const cookie{};
    mg::BufferProperties properties { geom::Size{1,1}, mir_pixel_format_abgr_8888, mg::BufferUsage::software};
};
}

TEST_F(AbstractShell, open_session_adds_session_to_window_manager)
{
    std::shared_ptr<ms::Session> new_session;

    InSequence s;
    EXPECT_CALL(*wm, add_session(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = shell.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    EXPECT_EQ(session, new_session);
}

TEST_F(AbstractShell, close_session_removes_session_from_window_manager)
{
    std::shared_ptr<ms::Session> old_session;

    InSequence s;
    EXPECT_CALL(*wm, remove_session(_)).WillOnce(SaveArg<0>(&old_session));

    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    shell.close_session(session);
    EXPECT_EQ(session, old_session);
}

TEST_F(AbstractShell, close_session_notifies_session_event_sink)
{
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

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
    EXPECT_CALL(surface_factory, create_surface(_, _, _)).
        WillOnce(Return(mt::fake_shared(surface1))).
        WillOnce(Return(mt::fake_shared(surface2))).
        WillOnce(Return(mt::fake_shared(surface3)));

    auto const session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const created_surface1 = shell.create_surface(session,
        ms::a_surface().with_buffer_stream(session->create_buffer_stream(properties)), nullptr);
    auto const created_surface2 = shell.create_surface(session,
        ms::a_surface().with_buffer_stream(session->create_buffer_stream(properties)), nullptr);
    auto const created_surface3 = shell.create_surface(session,
        ms::a_surface().with_buffer_stream(session->create_buffer_stream(properties)), nullptr);

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
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    EXPECT_CALL(*wm, add_surface(session, Ref(params), _));

    shell.create_surface(session, params, nullptr);
}

TEST_F(AbstractShell, create_surface_allows_window_manager_to_set_create_parameters)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto placed_params = params;
    placed_params.size.width = geom::Width{100};

    EXPECT_CALL(surface_stack, add_surface(_,placed_params.input_mode));

    EXPECT_CALL(*wm, add_surface(session, Ref(params), _)).WillOnce(Invoke(
        [&](std::shared_ptr<ms::Session> const& session,
            ms::SurfaceCreationParameters const&,
            std::function<std::shared_ptr<ms::Surface>(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const&)> const& build)
            { return build(session, placed_params); }));

    shell.create_surface(session, params, nullptr);
}

TEST_F(AbstractShell, create_surface_sets_surfaces_session)
{
    auto const mock_surface = std::make_shared<mtd::MockSurface>();

    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    EXPECT_CALL(surface_factory, create_surface(session, _, _)).
        WillOnce(Return(mock_surface));

    auto params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));

    shell.create_surface(session, params, nullptr);
}

TEST_F(AbstractShell, destroy_surface_removes_surface_from_window_manager)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));

    auto const surface = shell.create_surface(session, params, nullptr);

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
    xkb_keysym_t const key_code{0};
    int const scan_code{0};
    MirInputEventModifiers const modifiers{mir_input_event_modifier_none};

    auto const event = mir::events::make_event(
        mir_input_event_type_key,
        event_timestamp,
        cookie,
        action,
        key_code,
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

    auto const event = mir::events::make_event(
        mir_input_event_type_touch,
        event_timestamp,
        cookie,
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

    auto const event = mir::events::make_event(
        mir_input_event_type_pointer,
        event_timestamp,
        cookie,
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

TEST_F(AbstractShell, setting_surface_state_is_handled_by_window_manager)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto const params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto const surface = shell.create_surface(session, params, nullptr);

    MirWindowState const state{mir_window_state_fullscreen};

    EXPECT_CALL(*wm, set_surface_attribute(session, surface, mir_window_attrib_state, state))
        .WillOnce(WithArg<1>(Invoke([](std::shared_ptr<ms::Surface> const& surface)
             { surface->configure(mir_window_attrib_state, mir_window_state_maximized); return mir_window_state_maximized; })));

    EXPECT_CALL(mock_surface, configure(mir_window_attrib_state, mir_window_state_maximized));

    shell.set_surface_attribute(session, surface, mir_window_attrib_state, mir_window_state_fullscreen);
}

TEST_F(AbstractShell, as_focus_controller_set_focus_to_notifies_session_event_sink)
{
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

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
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto const params2 = ms::a_surface()
        .with_buffer_stream(session1->create_buffer_stream(properties));
    shell.create_surface(session, params, nullptr);
    shell.create_surface(session1, params2, nullptr);

    focus_controller.set_focus_to(session, {});

    EXPECT_CALL(session_event_sink, handle_focus_change(session1));

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell, as_focus_controller_focused_session_follows_focus)
{
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto const params2 = ms::a_surface()
        .with_buffer_stream(session1->create_buffer_stream(properties));
    shell.create_surface(session, params, nullptr);
    shell.create_surface(session1, params2, nullptr);

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
    auto const session0 = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto const session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());
    NiceMock<mtd::MockSurface> dummy_surface;
    ON_CALL(dummy_surface, size()).WillByDefault(Return(geom::Size{}));
    EXPECT_CALL(surface_factory, create_surface(_, _, _)).Times(AnyNumber())
        .WillOnce(Return(mt::fake_shared(dummy_surface)))
        .WillOnce(Return(mt::fake_shared(mock_surface)));

    auto const params = ms::a_surface()
        .with_buffer_stream(session0->create_buffer_stream(properties));
    auto const params2 = ms::a_surface()
        .with_buffer_stream(session1->create_buffer_stream(properties));
    auto const surface0 = shell.create_surface(session0, params, nullptr);
    auto const surface1 = shell.create_surface(session1, params2, nullptr);

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
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Surfaceless", std::shared_ptr<mf::EventSink>());
    auto session2 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());
    auto const params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto created_surface = shell.create_surface(session, params, nullptr);
    auto const params2 = ms::a_surface()
        .with_buffer_stream(session2->create_buffer_stream(properties));
    shell.create_surface(session2, params2, nullptr);

    focus_controller.set_focus_to(session, created_surface);

    EXPECT_CALL(session_event_sink, handle_focus_change(session2));

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell,
       as_focus_controller_focus_next_session_does_not_focus_any_session_if_no_session_has_surfaces)
{
    msh::FocusController& focus_controller = shell;
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Surfaceless", std::shared_ptr<mf::EventSink>());
    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);

    focus_controller.set_focus_to(session, surface);

    session->destroy_surface(surface);

    EXPECT_CALL(session_event_sink, handle_no_focus());

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell, modify_surface_with_only_streams_doesnt_call_into_wm)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);

    msh::SurfaceSpecification stream_modification;
    stream_modification.streams = std::vector<msh::StreamSpecification>{};

    EXPECT_CALL(*wm, modify_surface(_,_,_)).Times(0);

    shell.modify_surface(session, surface, stream_modification);
}

TEST_F(AbstractShell, modify_surface_does_not_call_wm_for_empty_changes)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));

    auto surface = shell.create_surface(session, creation_params, nullptr);

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
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);
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
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);
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
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);
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
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);
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
        shell.open_session(__LINE__, "empty_session", std::shared_ptr<mf::EventSink>());

    {
        std::shared_ptr<ms::Session> another_session =
            shell.open_session(__LINE__, "another_session", std::shared_ptr<mf::EventSink>());

        auto creation_params = ms::a_surface()
            .with_buffer_stream(another_session->create_buffer_stream(properties));
        auto surface = shell.create_surface(another_session, creation_params, nullptr);

        shell.set_focus_to(another_session, surface);

        shell.close_session(another_session);
    }

    shell.focus_next_session();
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

    auto session = shell.open_session(__LINE__, "some", std::shared_ptr<mf::EventSink>());
    auto creation_params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surface = shell.create_surface(session, creation_params, nullptr);

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(session, surface);
}
