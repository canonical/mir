/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_GL_CONFIG_H_
#define MIR_TEST_DOUBLES_MOCK_GL_CONFIG_H_

#include "mir/graphics/gl_config.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockGLConfig : public graphics::GLConfig
{
    MOCK_METHOD(int, depth_buffer_bits, (), (const));
    MOCK_METHOD(int, stencil_buffer_bits, (), (const));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_GL_CONFIG_H_ */
