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

#include "src/platform/graphics/android/overlay_gl_compositor.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/gl_program_factory.h"
#include "mir_test_doubles/mock_gl.h"
#include <gtest/gtest.h>
#include <mir_test/gmock_fixes.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;

namespace
{

class MockGLProgramFactory : public mg::GLProgramFactory
{
public:
    MOCK_CONST_METHOD2(create_gl_program,
        std::unique_ptr<mg::GLProgram>(std::string const&, std::string const&));
};

class MockContext : public mg::GLContext
{
public:
    MOCK_CONST_METHOD0(make_current, void());
    MOCK_CONST_METHOD0(release_current, void());
};

class OverlayCompositor : public ::testing::Test
{
public:
    testing::NiceMock<MockGLProgramFactory> mock_gl_program_factory;
    testing::NiceMock<MockContext> mock_context;
    testing::NiceMock<mtd::MockGL> mock_gl;
};

}

TEST_F(OverlayCompositor, compiles_in_constructor)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(mock_context, make_current());
    EXPECT_CALL(mock_gl_program_factory, create_gl_program(_,_));
    EXPECT_CALL(mock_context, release_current());

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context);
}
