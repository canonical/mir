/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
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
    MOCK_METHOD1(handle_input, void(std::shared_ptr<MirEvent> const&));
    MOCK_METHOD1(confine_pointer, void(mir::geometry::Point&));
    MOCK_CONST_METHOD0(bounding_rectangle, mir::geometry::Rectangle());
    MOCK_CONST_METHOD1(output_info, mir::input::OutputInfo(uint32_t));
    MOCK_METHOD1(key_state, void(std::vector<uint32_t> const&));
    MOCK_METHOD1(pointer_state, void(MirPointerButtons));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_SINK_H_
