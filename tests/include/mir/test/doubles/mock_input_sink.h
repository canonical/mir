/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
    MOCK_METHOD1(handle_input, void(MirEvent&));
    MOCK_METHOD1(confine_pointer, void(mir::geometry::Point&));
    MOCK_CONST_METHOD0(bounding_rectangle, mir::geometry::Rectangle());
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_SINK_H_
