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

#ifndef MIR_TEST_DOUBLES_MOCK_SLOW_KEYS_TRANSFORMER_H
#define MIR_TEST_DOUBLES_MOCK_SLOW_KEYS_TRANSFORMER_H

#include "mir/shell/slow_keys_transformer.h"

#include <gmock/gmock.h>

namespace mir::test::doubles
{
class MockSlowKeysTransformer: public mir::shell::SlowKeysTransformer
{
public:
    MOCK_METHOD(void, on_key_down,(std::function<void(MirKeyboardEvent const*)>&&), (override));
    MOCK_METHOD(void, on_key_rejected,(std::function<void(MirKeyboardEvent const*)>&&), (override));
    MOCK_METHOD(void, on_key_accepted,(std::function<void(MirKeyboardEvent const*)>&&), (override));
    MOCK_METHOD(void, delay,(std::chrono::milliseconds), (override));
    MOCK_METHOD(bool, transform_input_event, (EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&), (override));
};
}

#endif // MIR_TEST_DOUBLES_MOCK_SLOW_KEYS_TRANSFORMER_H
