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

#ifndef MIR_TEST_DOUBLES_MOCK_MOUSEKEYS_TRANSFORMER_H
#define MIR_TEST_DOUBLES_MOCK_MOUSEKEYS_TRANSFORMER_H

#include "mir/shell/mousekeys_transformer.h"

#include <gmock/gmock.h>

namespace mir::test::doubles
{
class MockMouseKeysTransformer: public mir::shell::MouseKeysTransformer
{
public:
    MockMouseKeysTransformer() = default;
    MOCK_METHOD(void, keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, max_speed, (double x_axis, double y_axis), (override));
    MOCK_METHOD(bool, transform_input_event, (EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&), (override));
};
}
#endif // MIR_TEST_DOUBLES_MOCK_MOUSEKEYS_TRANSFORMER_H
