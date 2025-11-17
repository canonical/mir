/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SINK_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SINK_H_

#include "mir/input/input_sink.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputSink : mir::input::InputSink
{
    MOCK_METHOD(void, handle_input, (std::shared_ptr<MirEvent> const&), ());
    MOCK_METHOD(void, confine_pointer, (mir::geometry::Point&), ());
    MOCK_METHOD(mir::geometry::Rectangle, bounding_rectangle, (), (const));
    MOCK_METHOD(mir::input::OutputInfo, output_info, (uint32_t), (const));
    MOCK_METHOD(void, key_state, (std::vector<uint32_t> const&), ());
    MOCK_METHOD(void, pointer_state, (MirPointerButtons), ());
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_SINK_H_
