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

#ifndef MIR_TEST_DOUBLES_MOCK_FRAMEBUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_FRAMEBUFFER_H_

#include "mir/graphics/platform.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockFramebuffer : public graphics::Framebuffer
{
    MOCK_METHOD(graphics::NativeBufferBase*, native_buffer_base, (), (override));
    MOCK_METHOD(MirPixelFormat, pixel_format, (), (const override));
    MOCK_METHOD(geometry::Size, size, (), (const override));
};
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_FRAMEBUFFER_H_
