/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/platform/graphics/android/fallback_overlay_gl_program.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{

class OverlayCompositor : public ::testing::Test
{
    testing::NiceMock<MockGLProgramFactory> mock_gl_program_factory;
    testing::NiceMock<mtd::MockGL> mock_gl;
};

}

TEST_F(OverlayCompositor, compiles_in_constructor)
{
    using namespace testing;
    EXPECT_CALL(mock_gl_program_factory, create_gl_program(_,_))
        .Times(1);
    mga::GLOverlayCompositor compositor(mock_gl_program_factory)
}
