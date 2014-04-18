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
#include "mir_test_doubles/mock_gl_program_factory.h"
#include "mir_test_doubles/mock_gl.h"
#include <gtest/gtest.h>

#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
#define GLM_HAS_INITIALIZER_LISTS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
namespace
{

class MockContext : public mg::GLContext
{
public:
    MOCK_CONST_METHOD0(make_current, void());
    MOCK_CONST_METHOD0(release_current, void());
};

class OverlayCompositor : public ::testing::Test
{
public:
    OverlayCompositor()
    {
        using namespace testing;
        ON_CALL(mock_gl_program_factory,create_gl_program(_,_))
            .WillByDefault(Invoke([](std::string const a, std::string const b)
                { return std::unique_ptr<mg::GLProgram>(new mg::GLProgram(a.c_str(),b.c_str())); }));

        ON_CALL(mock_gl, glGetUniformLocation(_, StrEq("display_transform")))
            .WillByDefault(Return(display_transform_uniform_loc));
    }

    GLint const display_transform_uniform_loc{1};

    testing::NiceMock<mtd::MockGLProgramFactory> mock_gl_program_factory;
    testing::NiceMock<MockContext> mock_context;
    testing::NiceMock<mtd::MockGL> mock_gl;
    geom::Rectangle dummy_screen_pos{geom::Point{0,0}, geom::Size{200,200}};
};

}

TEST_F(OverlayCompositor, compiles_and_sets_up_gl_program)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(mock_context, make_current());
    EXPECT_CALL(mock_gl_program_factory, create_gl_program(_,_));
    EXPECT_CALL(mock_gl, glGetUniformLocation(_, StrEq("display_transform")));
    EXPECT_CALL(mock_context, release_current());

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
}

TEST_F(OverlayCompositor, sets_up_orthographic_matrix_based_on_screen_size)
{
    using namespace testing;
    geom::Rectangle screen_pos{geom::Point{100,200}, geom::Size{800,600}};

    glm::mat4 expected_matrix(1.0);
    expected_matrix = glm::translate(glm::mat4(1.0), 
                                     glm::vec3{screen_pos.top_left.x.as_int(),
                                               screen_pos.top_left.y.as_int(),
                                               1.0});
    expected_matrix = glm::scale(glm::mat4(1.0),
                                 glm::vec3{screen_pos.size.width.as_int(),
                                           screen_pos.size.height.as_int(),
                                           1.0});

    glm::mat4 matrix(0.0);
    EXPECT_CALL(mock_gl, glUniformMatrix4fv(display_transform_uniform_loc, 1, GL_FALSE, _))
        .WillOnce(SaveArgPointee<3>(glm::value_ptr(matrix)));

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context, screen_pos);
    EXPECT_EQ(expected_matrix, matrix);
}
