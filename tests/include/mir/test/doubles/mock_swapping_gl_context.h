/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SWAPPING_GL_CONTEXT_H_
#define MIR_TEST_DOUBLES_MOCK_SWAPPING_GL_CONTEXT_H_

#include "src/platforms/android/server/gl_context.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSwappingGLContext : public graphics::android::SwappingGLContext
{
    MOCK_CONST_METHOD0(swap_buffers, void());
    MOCK_CONST_METHOD0(make_current, void());
    MOCK_CONST_METHOD0(release_current, void());
    MOCK_CONST_METHOD0(last_rendered_buffer, std::shared_ptr<graphics::Buffer>());
};

}
}
}
#endif // MIR_TEST_DOUBLES_MOCK_COMPOSITING_CRITERIA_H_
