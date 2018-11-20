/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_GL_CONFIG_H_
#define MIR_TEST_DOUBLES_NULL_GL_CONFIG_H_

#include "mir/graphics/gl_config.h"

namespace mir
{
namespace  test
{
namespace doubles
{
class NullGLConfig : public mir::graphics::GLConfig
{
public:
    int depth_buffer_bits() const override
    {
        return 0;
    }

    int stencil_buffer_bits() const override
    {
        return 0;
    }
};
}
}
}

#endif //MIR_TEST_DOUBLES_NULL_GL_CONFIG_H_
