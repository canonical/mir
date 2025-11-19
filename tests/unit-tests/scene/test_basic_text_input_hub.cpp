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

#include "src/server/scene/basic_text_input_hub.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace ms = mir::scene;
namespace mt = mir::test;

struct MockStateObserver : ms::TextInputStateObserver
{
    MOCK_METHOD(void, activated, (ms::TextInputStateSerial, bool, ms::TextInputState const&), (override));
    MOCK_METHOD(void, deactivated, (), (override));
    MOCK_METHOD(void, show_input_panel, (), (override));
    MOCK_METHOD(void, hide_input_panel, (), (override));
};

struct MockChangeHandler : ms::TextInputChangeHandler
{
    MOCK_METHOD(void, text_changed, (ms::TextInputChange const&), (override));
};

struct BasicTextInputHubTest : Test
{
    ms::BasicTextInputHub hub;
    StrictMock<MockStateObserver> state_observer;
    std::shared_ptr<MockStateObserver> state_observer_ptr = mt::fake_shared(state_observer);
    StrictMock<MockChangeHandler> change_handler;
    std::shared_ptr<MockChangeHandler> change_handler_ptr = mt::fake_shared(change_handler);
    ms::TextInputState state;
    ms::TextInputChange change{{}};
};

TEST_F(BasicTextInputHubTest, gives_different_valid_serial_for_each_state)
{
    state.surrounding_text = "a";
    auto const serial_1 = hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_THAT(serial_1, Ne(decltype(serial_1){}));
    auto const serial_2 = hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_THAT(serial_1, Ne(serial_2));
}

TEST_F(BasicTextInputHubTest, observer_is_deactivated_when_added_with_no_active_state)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
}

TEST_F(BasicTextInputHubTest, observer_is_activated_with_correct_serial)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
    ms::TextInputStateSerial activated_serial;
    EXPECT_CALL(state_observer, activated(_, _, _))
        .WillOnce(SaveArg<0>(&activated_serial));
    auto const serial_1 = hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_THAT(activated_serial, Eq(serial_1));
}

TEST_F(BasicTextInputHubTest, observer_is_activated_with_correct_state)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
    state.surrounding_text = "a";
    ms::TextInputState activated_state;
    EXPECT_CALL(state_observer, activated(_, _, _))
        .WillOnce(SaveArg<2>(&activated_state));
    hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_THAT(activated_state.surrounding_text, Eq(state.surrounding_text));
}

TEST_F(BasicTextInputHubTest, observer_is_activated_when_added_if_hub_has_active_state)
{
    hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_CALL(state_observer, activated(_, _, _));
    hub.register_interest(state_observer_ptr);
}

TEST_F(BasicTextInputHubTest, observer_is_initially_activated_with_correct_serial)
{
    auto const serial_1 = hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_CALL(state_observer, activated(Eq(serial_1), _, _));
    hub.register_interest(state_observer_ptr);
}

TEST_F(BasicTextInputHubTest, observer_is_initially_activated_with_correct_state)
{
    state.surrounding_text = "a";
    hub.set_handler_state(change_handler_ptr, false, state);
    ms::TextInputState activated_state;
    EXPECT_CALL(state_observer, activated(_, _, _))
        .WillOnce(SaveArg<2>(&activated_state));
    hub.register_interest(state_observer_ptr);
    EXPECT_THAT(activated_state.surrounding_text, Eq(state.surrounding_text));
}

TEST_F(BasicTextInputHubTest, observer_sees_new_input_field_on_handler_change)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
    EXPECT_CALL(state_observer, activated(_, true, _));
    hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_CALL(state_observer, activated(_, true, _));
    MockChangeHandler other_handler;
    hub.set_handler_state(mt::fake_shared(other_handler), false, state);
}

TEST_F(BasicTextInputHubTest, observer_sees_new_input_field_when_new_input_field_specified)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
    EXPECT_CALL(state_observer, activated(_, true, _));
    hub.set_handler_state(change_handler_ptr, true, state);
    EXPECT_CALL(state_observer, activated(_, true, _));
    hub.set_handler_state(change_handler_ptr, true, state);
}

TEST_F(BasicTextInputHubTest, observer_does_not_see_new_input_field_for_same_handler)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
    EXPECT_CALL(state_observer, activated(_, true, _));
    hub.set_handler_state(change_handler_ptr, false, state);
    EXPECT_CALL(state_observer, activated(_, false, _));
    hub.set_handler_state(change_handler_ptr, false, state);
}

TEST_F(BasicTextInputHubTest, handler_sees_change)
{
    EXPECT_CALL(state_observer, deactivated());
    hub.register_interest(state_observer_ptr);
    EXPECT_CALL(state_observer, activated(_, _, _));
    auto const serial_1 = hub.set_handler_state(change_handler_ptr, false, state);
    ms::TextInputChange actual_change{{}};
    EXPECT_CALL(change_handler, text_changed(_))
        .WillOnce(SaveArg<0>(&actual_change));
    change.serial = serial_1;
    change.commit_text = "a";
    hub.text_changed(change);
    EXPECT_THAT(actual_change.serial, Eq(serial_1));
    EXPECT_THAT(actual_change.commit_text, Eq(change.commit_text));
}

TEST_F(BasicTextInputHubTest, handler_does_not_see_previously_sent_change)
{
    change.commit_text = "a";
    hub.text_changed(change);
    EXPECT_CALL(change_handler, text_changed(_))
        .Times(0);
    hub.set_handler_state(change_handler_ptr, false, state);
}
