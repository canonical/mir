/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_INITIALIZER_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_INITIALIZER_H_

#include "mir/graphics/buffer_initializer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockBufferInitializer : public graphics::BufferInitializer
{
public:
    MOCK_METHOD1(operator_call, void(graphics::Buffer& buffer));

    void operator()(graphics::Buffer& buffer)
    {
        operator_call(buffer);
    }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_BUFFER_INITIALIZER_H_ */
