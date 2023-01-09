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

#ifndef MIR_TEST_GEOMETRY_MATCHERS_H_
#define MIR_TEST_GEOMETRY_MATCHERS_H_

#include <gmock/gmock.h>

namespace mir
{
namespace test
{

MATCHER_P(RectTopLeftEq, expected_top_left, "")
{
    return arg.top_left == expected_top_left;
}

MATCHER_P(RectSizeEq, expected_size, "")
{
    return arg.size == expected_size;
}

}
}

#endif // MIR_TEST_GEOMETRY_MATCHERS_H_
