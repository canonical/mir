/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_DOUBLES_MOCK_ACCESSIBILITY_MANAGER_H_
#define MIR_TEST_DOUBLES_MOCK_ACCESSIBILITY_MANAGER_H_

#include "mir/shell/accessibility_manager.h"

#include <gmock/gmock.h>

class MockAccessibilityManager: public mir::shell::AccessibilityManager
{
public:
    MOCK_METHOD(void, register_keyboard_helper, (std::shared_ptr<mir::shell::KeyboardHelper> const&), (override));
    MOCK_METHOD(std::optional<int>, repeat_rate, (), (const override));
    MOCK_METHOD(int, repeat_delay, (), (const override));
    MOCK_METHOD(void, repeat_rate_and_delay, (std::optional<int> new_rate, std::optional<int> new_delay), (override));
    MOCK_METHOD(void, notify_helpers, (), (const override));
    MOCK_METHOD(void, cursor_scale, (float), (override));
    MOCK_METHOD(void, mousekeys_enabled, (bool on), (override));
    MOCK_METHOD(void, mousekeys_keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, max_speed, (double x_axis, double y_axis), (override));
};

#endif // MIR_TEST_DOUBLES_MOCK_ACCESSIBILITY_MANAGER_H_
