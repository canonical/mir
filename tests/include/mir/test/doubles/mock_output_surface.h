/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_OUTPUT_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_OUTPUT_SURFACE_H_

#include <gmock/gmock.h>

#include "mir/graphics/buffer.h"
#include "mir/renderer/gl/gl_surface.h"

namespace mir::test::doubles
{
class MockOutputSurface : public mir::graphics::gl::OutputSurface
{
public:
    MOCK_METHOD(void, bind, (), (override));
    MOCK_METHOD(void, make_current, (), (override));
    MOCK_METHOD(void, release_current, (), (override));
    MOCK_METHOD(std::unique_ptr<graphics::Buffer>, commit, (), (override));
    MOCK_METHOD(mir::geometry::Size, size, (), (const override));
    MOCK_METHOD(Layout, layout, (), (const override));
};
}

#endif //MIR_TEST_DOUBLES_MOCK_OUTPUT_SURFACE_H_
