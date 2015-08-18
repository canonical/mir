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
 * Authored by: ALan Griffiths <alan.griffiths@canonical.com>
 */

#include "mir/shell/abstract_shell.h"

#include "mir/events/event_builders.h"
#include "mir/scene/session.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface_factory.h"

#include "src/server/scene/default_session_container.h"
#include "src/server/scene/session_event_sink.h"
#include "src/server/scene/session_manager.h"

#include "mir/test/doubles/mock_window_manager.h"
#include "mir/test/doubles/mock_surface_coordinator.h"
#include "mir/test/doubles/mock_session_listener.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/null_snapshot_strategy.h"
#include "mir/test/doubles/null_prompt_session_manager.h"
#include "mir/test/doubles/stub_input_targeter.h"
#include "mir/test/doubles/stub_buffer_stream_factory.h"
#include "mir/test/doubles/null_application_not_responding_detector.h"

#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
using namespace ::testing;

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

    MOCK_METHOD1(set_focus_to, void (std::shared_ptr<ms::Session> const& focus));

    void unmocked_set_focus_to(std::shared_ptr<ms::Session> const& focus)
    { ms::SessionManager::set_focus_to(focus); }
};

struct MockSurfaceFactory : public ms::SurfaceFactory
{
    MOCK_METHOD2(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<mir::compositor::BufferStream> const&,
        ms::SurfaceCreationParameters const&));
};

using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct AbstractShell : Test
{
    NiceMock<mtd::MockSurface> mock_surface;
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    NiceMock<MockSessionContainer> session_container;
    NiceMock<MockSessionEventSink> session_event_sink;
    NiceMock<mtd::MockSessionListener> session_listener;
    NiceMock<MockSurfaceFactory> surface_factory;

    NiceMock<MockSessionManager> session_manager{
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(surface_factory),
        std::make_shared<mtd::StubBufferStreamFactory>(),
        mt::fake_shared(session_container),
        std::make_shared<mtd::NullSnapshotStrategy>(),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(session_listener),
        std::make_shared<mtd::NullANRDetector>()};

    mtd::StubInputTargeter input_targeter;
    std::shared_ptr<NiceMockWindowManager> wm;

    msh::AbstractShell shell{
        mt::fake_shared(input_targeter),
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(session_manager),
        std::make_shared<mtd::NullPromptSessionManager>(),
        [this](msh::FocusController*) { return wm = std::make_shared<NiceMockWindowManager>(); }};

    void SetUp() override
    {
        ON_CALL(session_container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
        ON_CALL(session_manager, set_focus_to(_)).
            WillByDefault(Invoke(&session_manager, &MockSessionManager::unmocked_set_focus_to));
        ON_CALL(surface_factory, create_surface(_,_))
            .WillByDefault(Return(mt::fake_shared(mock_surface)));
    }

    std::chrono::nanoseconds const event_timestamp = std::chrono::nanoseconds(0);
};
}

TEST_F(AbstractShell, open_session_adds_session_to_window_manager)
{
    std::shared_ptr<ms::Session> new_session;

    InSequence s;
    EXPECT_CALL(session_container, insert_session(_)).Times(1);
    EXPECT_CALL(*wm, add_session(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = shell.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    EXPECT_EQ(session, new_session);
}

TEST_F(AbstractShell, close_session_removes_session_from_window_manager)
{
    std::shared_ptr<ms::Session> old_session;

    InSequence s;
    EXPECT_CALL(session_container, insert_session(_));
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

TEST_F(AbstractShell, create_surface_provides_create_parameters_to_window_manager)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface();

    EXPECT_CALL(*wm, add_surface(session, Ref(params), _));

    shell.create_surface(session, params);
}

TEST_F(AbstractShell, create_surface_allows_window_manager_to_set_create_parameters)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface();
    auto placed_params = params;
    placed_params.size.width = geom::Width{100};

    EXPECT_CALL(surface_coordinator, add_surface(_,placed_params.depth,placed_params.input_mode,_));

    EXPECT_CALL(*wm, add_surface(session, Ref(params), _)).WillOnce(Invoke(
        [&](std::shared_ptr<ms::Session> const& session,
            ms::SurfaceCreationParameters const&,
            std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const&)> const& build)
            { return build(session, placed_params); }));

    shell.create_surface(session, params);
}

TEST_F(AbstractShell, destroy_surface_removes_surface_from_window_manager)
{
    auto const params = ms::a_surface();
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto const surface_id = shell.create_surface(session, params);
    auto const surface = session->surface(surface_id);

    EXPECT_CALL(*wm, remove_surface(session, WeakPtrTo(surface)));

    shell.destroy_surface(session, surface_id);
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

    auto const event = mir::events::make_event(
        mir_input_event_type_pointer,
        event_timestamp,
        modifiers,
        action,
        buttons_pressed,
        x_axis_value,
        y_axis_value,
        hscroll_value,
        vscroll_value);

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

    auto const surface_id = shell.create_surface(session, ms::a_surface());
    auto const surface = session->surface(surface_id);

    MirSurfaceState const state{mir_surface_state_fullscreen};

    EXPECT_CALL(*wm, set_surface_attribute(session, surface, mir_surface_attrib_state, state))
        .WillOnce(WithArg<1>(Invoke([](std::shared_ptr<ms::Surface> const& surface)
             { surface->configure(mir_surface_attrib_state, mir_surface_state_maximized); return mir_surface_state_maximized; })));

    EXPECT_CALL(mock_surface, configure(mir_surface_attrib_state, mir_surface_state_maximized));

    shell.set_surface_attribute(session, surface, mir_surface_attrib_state, mir_surface_state_fullscreen);
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
    focus_controller.set_focus_to(session, {});

    EXPECT_CALL(session_container, successor_of(session)).
        WillOnce(Return(session1));

    EXPECT_CALL(session_event_sink, handle_focus_change(session1));

    focus_controller.focus_next_session();
}

TEST_F(AbstractShell, as_focus_controller_focused_session_follows_focus)
{
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());
    EXPECT_CALL(session_container, successor_of(session1)).
        WillOnce(Return(session));

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

    EXPECT_CALL(surface_factory, create_surface(_,_)).Times(AnyNumber())
        .WillOnce(Return(mt::fake_shared(dummy_surface)))
        .WillOnce(Return(mt::fake_shared(mock_surface)));
    EXPECT_CALL(session_container, successor_of(session1)).
        WillOnce(Return(session0));


    auto const surface0_id = shell.create_surface(session0, ms::a_surface());
    auto const surface0 = session0->surface(surface0_id);
    auto const surface1_id = shell.create_surface(session1, ms::a_surface());
    auto const surface1 = session1->surface(surface1_id);

    msh::FocusController& focus_controller = shell;

    focus_controller.set_focus_to(session0, surface0);
    EXPECT_THAT(focus_controller.focused_surface(), Eq(surface0));
    focus_controller.set_focus_to(session1, surface1);
    EXPECT_THAT(focus_controller.focused_surface(), Eq(surface1));
    focus_controller.focus_next_session();
    EXPECT_THAT(focus_controller.focused_surface(), Eq(surface0));

    shell.destroy_surface(session0, surface0_id);
    shell.destroy_surface(session1, surface1_id);
    shell.close_session(session1);
    shell.close_session(session0);
}

TEST_F(AbstractShell, as_focus_controller_delegates_surface_at_to_surface_coordinator)
{
    auto const surface = mt::fake_shared(mock_surface);
    geom::Point const cursor{__LINE__, __LINE__};

    EXPECT_CALL(surface_coordinator, surface_at(cursor)).
        WillOnce(Return(surface));

    msh::FocusController& focus_controller = shell;

    EXPECT_THAT(focus_controller.surface_at(cursor), Eq(surface));
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

TEST_F(AbstractShell, as_focus_controller_delegates_raise_to_surface_coordinator)
{
    msh::SurfaceSet const surfaces{mt::fake_shared(mock_surface)};

    EXPECT_CALL(surface_coordinator, raise(surfaces));

    msh::FocusController& focus_controller = shell;

    focus_controller.raise(surfaces);
}
