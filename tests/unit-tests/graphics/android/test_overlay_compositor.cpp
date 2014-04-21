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
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_swapping_gl_context.h"
#include <gtest/gtest.h>

#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
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

        ON_CALL(mock_gl, glGetShaderiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetProgramiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetUniformLocation(_, StrEq("display_transform")))
            .WillByDefault(Return(display_transform_uniform_loc));
        ON_CALL(mock_gl, glGetUniformLocation(_, StrEq("position")))
            .WillByDefault(Return(position_attr_loc));
    }

    GLint const display_transform_uniform_loc{1};
    GLint const position_attr_loc{2};

    testing::NiceMock<mtd::MockGLProgramFactory> mock_gl_program_factory;
    testing::NiceMock<MockContext> mock_context;
    testing::NiceMock<mtd::MockGL> mock_gl;
    geom::Rectangle dummy_screen_pos{geom::Point{0,0}, geom::Size{500,400}};
};

}

TEST_F(OverlayCompositor, compiles_and_sets_up_gl_program)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(mock_context, make_current());
    EXPECT_CALL(mock_gl_program_factory, create_gl_program(_,_));
    EXPECT_CALL(mock_gl, glUseProgram(_));
    EXPECT_CALL(mock_gl, glGetUniformLocation(_, StrEq("display_transform")));
    EXPECT_CALL(mock_gl, glGetUniformLocation(_, StrEq("position")));
    EXPECT_CALL(mock_gl, glUseProgram(0));
    EXPECT_CALL(mock_context, release_current());

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
}


MATCHER_P(Matches4x4Matrix, value, "matches expected 4x4 matrix")
{
    for(auto i=0; i < 16; i++)
        EXPECT_THAT(arg[i], testing::FloatEq(value[i]));
    return !(::testing::Test::HasFailure());
}

TEST_F(OverlayCompositor, sets_up_orthographic_matrix_based_on_screen_size)
{
    using namespace testing;
    geom::Size sz{800,600};
    geom::Point pt{100,200};
    geom::Rectangle screen_pos{pt, sz};
    float inv_w = 1.0/sz.width.as_int();
    float inv_h = 1.0/sz.height.as_int();
    float x = static_cast<float>(pt.x.as_int() + sz.width.as_int()/2);
    float y = static_cast<float>(pt.y.as_int() + sz.height.as_int()/2);

    float expected_matrix[]{
        inv_w, 0.0    , 0.0, 0.0,
        0.0  , inv_h  , 0.0, 0.0,
        0.0  , 0.0    , 1.0, 0.0,
        -x   , -y     , 0.0, 1.0
    };

    EXPECT_CALL(mock_gl, glUniformMatrix4fv(
        display_transform_uniform_loc, 1, GL_FALSE, Matches4x4Matrix(expected_matrix)));

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context, screen_pos);
}

struct Vertex 
{
    float x;
    float y;
};

Vertex to_vertex(geom::Point const& pos)
{
    return {
        static_cast<float>(pos.x.as_int()),
        static_cast<float>(pos.y.as_int())
    };
}

geom::Point top_right(geom::Rectangle const& rect)
{
    return rect.top_left + geom::DeltaX{rect.size.width.as_int()};
}
geom::Point bottom_left(geom::Rectangle const& rect)
{
    return rect.top_left + geom::DeltaY{rect.size.height.as_int()};
}
geom::Point bottom_right(geom::Rectangle const& rect)
{
    return rect.top_left + geom::DeltaX{rect.size.width.as_int()} + geom::DeltaY{rect.size.height.as_int()};
}

MATCHER_P(MatchesVertices, vertices, "matches vertices")
{
    int i{0};
    auto arg_vertices = static_cast<GLfloat const*>(arg);
    for(auto const& vert : vertices)
    { 
        EXPECT_THAT(arg_vertices[i++], testing::FloatEq(vert.x));
        EXPECT_THAT(arg_vertices[i++], testing::FloatEq(vert.y));
    } 
    return !(::testing::Test::HasFailure());
}

TEST_F(OverlayCompositor, computes_vertex_coordinates_correctly)
{
    using namespace testing;
    mtd::MockSwappingGLContext mock_context_s;
    geom::Rectangle rect1{{100,200},{50, 60}};
    geom::Rectangle rect2{{150,250},{150, 90}};
    
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubRenderable>(rect1), 
        std::make_shared<mtd::StubRenderable>(rect2)
    };

    std::vector<Vertex> expected_vertices1 {
        to_vertex(rect1.top_left),
        to_vertex(rect1.top_right()),
        to_vertex(rect1.bottom_left()),
        to_vertex(rect1.bottom_right()),
    };
    std::vector<Vertex> expected_vertices2 {
        to_vertex(rect2.top_left),
        to_vertex(rect2.top_right()),
        to_vertex(rect2.bottom_left()),
        to_vertex(rect2.bottom_right()),
    };

    InSequence seq;
    EXPECT_CALL(mock_gl, glVertexAttribPointer(
        position_attr_loc, 2, GL_FLOAT, GL_FALSE, 0, MatchesVertices(expected_vertices1)));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(
        position_attr_loc, 2, GL_FLOAT, GL_FALSE, 0, MatchesVertices(expected_vertices2)));

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
    glprogram.render(renderlist, mock_context_s);
}

TEST_F(OverlayCompositor, executes_render_in_sequence)
{
    using namespace testing;
    mtd::MockSwappingGLContext mock_context_s;
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubRenderable>(), 
        std::make_shared<mtd::StubRenderable>()
    };

    InSequence seq;
    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(position_attr_loc));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_loc, 2, GL_FLOAT, GL_FALSE, 0, _));
    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_loc, 2, GL_FLOAT, GL_FALSE, 0, _));
    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(position_attr_loc));
    EXPECT_CALL(mock_context_s, swap_buffers());

    mga::OverlayGLProgram glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
    glprogram.render(renderlist, mock_context_s);
}


