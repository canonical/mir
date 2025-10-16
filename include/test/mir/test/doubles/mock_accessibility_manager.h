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

#ifndef MIR_TEST_DOUBLES_MOCK_ACCESSIBILITY_MANAGER_H
#define MIR_TEST_DOUBLES_MOCK_ACCESSIBILITY_MANAGER_H

#include <mir/shell/accessibility_manager.h>
#include <mir/test/doubles/mock_hover_click_transformer.h>
#include <mir/test/doubles/mock_mousekeys_transformer.h>
#include <mir/test/doubles/mock_slow_keys_transformer.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockAccessibilityManager: public mir::shell::AccessibilityManager
{
public:
    MockAccessibilityManager()
    {
        ON_CALL(*this, mousekeys()).WillByDefault(testing::ReturnRef(mousekeys_transformer));
        ON_CALL(*this, slow_keys()).WillByDefault(ReturnRef(slow_keys_transformer));
        ON_CALL(*this, hover_click()).WillByDefault(ReturnRef(hover_click_transformer));
    }

    MOCK_METHOD(void, register_keyboard_helper, (std::shared_ptr<mir::shell::KeyboardHelper> const&), (override));
    MOCK_METHOD(std::optional<int>, repeat_rate, (), (const, override));
    MOCK_METHOD(int, repeat_delay, (), (const, override));
    MOCK_METHOD(void, repeat_rate_and_delay, (std::optional<int> new_rate, std::optional<int> new_delay), (override));
    MOCK_METHOD(void, cursor_scale, (float), (override));
    MOCK_METHOD(void, mousekeys_enabled, (bool on), (override));
    MOCK_METHOD(mir::shell::MouseKeysTransformer&, mousekeys, (), (override));
    MOCK_METHOD(void, simulated_secondary_click_enabled, (bool enabled), (override));
    MOCK_METHOD(mir::shell::SimulatedSecondaryClickTransformer&, simulated_secondary_click, (), (override));
    MOCK_METHOD(void, hover_click_enabled, (bool), (override));
    MOCK_METHOD(mir::shell::HoverClickTransformer&, hover_click, (), (override));
    MOCK_METHOD(void, slow_keys_enabled, (bool enabled), (override));
    MOCK_METHOD(mir::shell::SlowKeysTransformer&, slow_keys, (), (override));
    MOCK_METHOD(void, sticky_keys_enabled, (bool), (override));
    MOCK_METHOD(mir::shell::StickyKeysTransformer&, sticky_keys, (), (override));

    testing::NiceMock<MockMouseKeysTransformer> mousekeys_transformer;
    testing::NiceMock<MockSlowKeysTransformer> slow_keys_transformer;
    testing::NiceMock<MockHoverClickTransformer> hover_click_transformer;
};
}
}
}
#endif // MIR_TEST_DOUBLES_MOCK_ACCESSIBILITY_MANAGER_H
