/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_TEST_MOCK_BUFFER_INITIALIZER_H_
#define MIR_TEST_MOCK_BUFFER_INITIALIZER_H_

#include "mir/graphics/buffer_initializer.h"

namespace mir
{
namespace graphics
{

class MockBufferInitializer : public BufferInitializer
{
public:
    MOCK_METHOD2(operator_call, void(compositor::Buffer& buffer, EGLClientBuffer client_buffer));

    void operator()(compositor::Buffer& buffer, EGLClientBuffer client_buffer)
    {
        operator_call(buffer, client_buffer);
    }
};

}
}

#endif /* MIR_TEST_MOCK_BUFFER_INITIALIZER_H_ */

