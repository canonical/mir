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

#include "src/platforms/android/server/hwc_fallback_gl_renderer.h"
#include "mir/graphics/gl_context.h"
#include "mir/gl/program_factory.h"
#include "mir/gl/primitive.h"
#include "mir/gl/texture.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/mock_swapping_gl_context.h"
#include "mir/test/doubles/stub_gl_program.h"
#include <gtest/gtest.h>
#include <mir/test/gmock_fixes.h>

#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mgl=mir::gl;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
namespace
{

class MockGLProgramFactory : public mgl::ProgramFactory
{
public:
    MOCK_CONST_METHOD2(create_gl_program,
        std::unique_ptr<mgl::Program>(std::string const&, std::string const&));
    MOCK_CONST_METHOD0(create_texture_cache, std::unique_ptr<mgl::TextureCache>());
};

class MockContext : public mg::GLContext
{
public:
    MOCK_CONST_METHOD0(make_current, void());
    MOCK_CONST_METHOD0(release_current, void());
};

struct MockTextureCache : public mgl::TextureCache
{
    MockTextureCache()
    {
        ON_CALL(*this, load(testing::_))
            .WillByDefault(testing::Return(std::make_shared<mgl::Texture>()));
    }
    MOCK_METHOD1(load, std::shared_ptr<mgl::Texture>(mg::Renderable const&));
    MOCK_METHOD0(invalidate, void());
    MOCK_METHOD0(drop_unused, void());
};

class StubTextureCache : public mgl::TextureCache
{
    std::shared_ptr<mgl::Texture> load(mg::Renderable const&)
    {
        return std::make_shared<mgl::Texture>();
    }
    void invalidate()
    {
    }
    void drop_unused()
    {
    }
};

class HWCFallbackGLRenderer : public ::testing::Test
{
public:
    HWCFallbackGLRenderer()
    {
        using namespace testing;
        ON_CALL(mock_gl_program_factory,create_gl_program(_,_))
            .WillByDefault(Invoke([](std::string const, std::string const)
                { return std::unique_ptr<mgl::Program>(new mtd::StubGLProgram); }));
        ON_CALL(mock_gl_program_factory,create_texture_cache())
            .WillByDefault(Invoke([]()
                { return std::unique_ptr<mgl::TextureCache>(new StubTextureCache); }));

        ON_CALL(mock_gl, glGetShaderiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetProgramiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetUniformLocation(_, StrEq("display_transform")))
            .WillByDefault(Return(display_transform_uniform_loc));
        ON_CALL(mock_gl, glGetUniformLocation(_, StrEq("tex")))
            .WillByDefault(Return(tex_uniform_loc));
        ON_CALL(mock_gl, glGetAttribLocation(_, StrEq("position")))
            .WillByDefault(Return(position_attr_loc));
        ON_CALL(mock_gl, glGetAttribLocation(_, StrEq("texcoord")))
            .WillByDefault(Return(texcoord_attr_loc));
        ON_CALL(mock_gl, glGenTextures(1,_))
            .WillByDefault(SetArgPointee<1>(texid));
    }

    geom::Displacement offset;
    GLint const display_transform_uniform_loc{1};
    GLint const position_attr_loc{2};
    GLint const texcoord_attr_loc{3};
    GLint const tex_uniform_loc{4};
    GLint const texid{5};
    size_t const stride{sizeof(mgl::Vertex)};

    testing::NiceMock<MockGLProgramFactory> mock_gl_program_factory;
    testing::NiceMock<mtd::MockSwappingGLContext> mock_swapping_context;
    testing::NiceMock<MockContext> mock_context;
    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockEGL> mock_egl;
    geom::Rectangle dummy_screen_pos{geom::Point{0,0}, geom::Size{500,400}};
};
}

TEST_F(HWCFallbackGLRenderer, compiles_and_sets_up_gl_program)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(mock_context, make_current());
    EXPECT_CALL(mock_gl_program_factory, create_gl_program(_,_));
    EXPECT_CALL(mock_gl, glUseProgram(_));
    EXPECT_CALL(mock_gl, glGetUniformLocation(_, StrEq("display_transform")));
    EXPECT_CALL(mock_gl, glGetAttribLocation(_, StrEq("position")));
    EXPECT_CALL(mock_gl, glGetAttribLocation(_, StrEq("texcoord")));
    EXPECT_CALL(mock_gl, glGetUniformLocation(_, StrEq("tex")));
    EXPECT_CALL(mock_gl, glUniform1i(tex_uniform_loc, 0));
    EXPECT_CALL(mock_gl, glUseProgram(0));
    EXPECT_CALL(mock_context, release_current());

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
}

MATCHER_P(Matches4x4Matrix, value, "matches expected 4x4 matrix")
{
    for(auto i=0; i < 16; i++)
        EXPECT_THAT(arg[i], testing::FloatEq(value[i]));
    return !(::testing::Test::HasFailure());
}

TEST_F(HWCFallbackGLRenderer, sets_up_orthographic_matrix_based_on_screen_size)
{
    using namespace testing;
    geom::Size sz{800,600};
    geom::Point pt{100,200};
    geom::Rectangle screen_pos{pt, sz};
    float inv_w = 2.0/sz.width.as_int();
    float inv_h = 2.0/sz.height.as_int() * -1.0;

    float expected_matrix[]{
        inv_w, 0.0    , 0.0, 0.0,
        0.0  , inv_h  , 0.0, 0.0,
        0.0  , 0.0    , 1.0, 0.0,
        -1.0 , 1.0    , 0.0, 1.0
    };

    EXPECT_CALL(mock_gl, glUniformMatrix4fv(
        display_transform_uniform_loc, 1, GL_FALSE, Matches4x4Matrix(expected_matrix)));

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, screen_pos);
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

MATCHER_P2(MatchesVertices, vertices, stride, "matches vertices")
{
    auto arg_vertices = static_cast<char const*>(arg);
    for(auto const& vert : vertices)
    {
        GLfloat *f = (GLfloat*) arg_vertices;
        EXPECT_THAT(f[0], testing::FloatEq(vert.x));
        EXPECT_THAT(f[1], testing::FloatEq(vert.y));
        arg_vertices+=stride;
    } 
    return !(::testing::Test::HasFailure());
}

TEST_F(HWCFallbackGLRenderer, computes_vertex_coordinates_correctly)
{
    using namespace testing;
    geom::Rectangle rect1{{100,200},{50, 60}};
    geom::Rectangle rect2{{150,250},{150, 90}};
    
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubRenderable>(rect1),
        std::make_shared<mtd::StubRenderable>(rect2)
    };

    std::vector<Vertex> expected_vertices1 {
        to_vertex(rect1.top_left),
        to_vertex(rect1.bottom_left()),
        to_vertex(rect1.top_right()),
        to_vertex(rect1.bottom_right()),
    };
    std::vector<Vertex> expected_vertices2 {
        to_vertex(rect2.top_left),
        to_vertex(rect2.bottom_left()),
        to_vertex(rect2.top_right()),
        to_vertex(rect2.bottom_right()),
    };

    InSequence seq;
    EXPECT_CALL(mock_gl, glVertexAttribPointer(
        position_attr_loc, 3, GL_FLOAT, GL_FALSE, stride, MatchesVertices(expected_vertices1, stride)));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(_,_,_,_,_,_))
        .Times(1);
    EXPECT_CALL(mock_gl, glVertexAttribPointer(
        position_attr_loc, 3, GL_FLOAT, GL_FALSE, stride, MatchesVertices(expected_vertices2, stride)));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(_,_,_,_,_,_))
        .Times(1);

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
    glprogram.render(renderlist, offset, mock_swapping_context);
}

TEST_F(HWCFallbackGLRenderer, computes_vertex_coordinates_correctly_with_offset)
{
    using namespace testing;
    geom::Displacement offset{100, 50};
    geom::Rectangle rect{{100,200},{50, 60}};
    geom::Rectangle offset_rect{{0, 150}, rect.size};
 
    mg::RenderableList renderlist{std::make_shared<mtd::StubRenderable>(rect)};
    std::vector<Vertex> expected_vertices {
        to_vertex(offset_rect.top_left),
        to_vertex(offset_rect.bottom_left()),
        to_vertex(offset_rect.top_right()),
        to_vertex(offset_rect.bottom_right()),
    };

    InSequence seq;
    EXPECT_CALL(mock_gl, glVertexAttribPointer(
        position_attr_loc, 3, GL_FLOAT, GL_FALSE, stride, MatchesVertices(expected_vertices, stride)));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(_,_,_,_,_,_))
        .Times(1);

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
    glprogram.render(renderlist, offset, mock_swapping_context);
}

TEST_F(HWCFallbackGLRenderer, computes_texture_coordinates_correctly)
{
    using namespace testing;
    geom::Rectangle rect1{{100,200},{50, 60}};
    geom::Rectangle rect2{{150,250},{150, 90}};
    
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubRenderable>(rect1), 
        std::make_shared<mtd::StubRenderable>(rect2)
    };

    std::vector<Vertex> expected_texcoord {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f}
    };

    EXPECT_CALL(mock_gl, glVertexAttribPointer(_,_,_,_,_,_)).Times(AnyNumber());
    EXPECT_CALL(mock_gl, glVertexAttribPointer(
        texcoord_attr_loc, 2, GL_FLOAT, GL_FALSE, stride, MatchesVertices(expected_texcoord, stride)))
        .Times(2);

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);
    glprogram.render(renderlist, offset, mock_swapping_context);
}

TEST_F(HWCFallbackGLRenderer, executes_render_in_sequence)
{
    using namespace testing;
    auto renderable1 = std::make_shared<mtd::StubRenderable>();
    auto renderable2 = std::make_shared<mtd::StubRenderable>();
    mg::RenderableList renderlist{ renderable1, renderable2 };

    std::unique_ptr<MockTextureCache> mock_texture_cache(new MockTextureCache);
    {
        InSequence seq;
        EXPECT_CALL(*mock_texture_cache, load(Ref(*renderable1))); 
        EXPECT_CALL(*mock_texture_cache, load(Ref(*renderable2))); 
        EXPECT_CALL(*mock_texture_cache, drop_unused());
    }
    ON_CALL(mock_gl_program_factory,create_texture_cache())
        .WillByDefault(Invoke([&](){
                return std::move(mock_texture_cache);
            }));

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);

    InSequence seq;
    EXPECT_CALL(mock_gl, glUseProgram(_));
    EXPECT_CALL(mock_gl, glClearColor(FloatEq(0.0), FloatEq(0.0), FloatEq(0.0), FloatEq(0.0)));
    EXPECT_CALL(mock_gl, glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    EXPECT_CALL(mock_gl, glClear(GL_COLOR_BUFFER_BIT));
    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(position_attr_loc));
    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(texcoord_attr_loc));

    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_loc, 3, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(texcoord_attr_loc, 2, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_loc, 3, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(texcoord_attr_loc, 2, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(texcoord_attr_loc));
    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(position_attr_loc));
    EXPECT_CALL(mock_swapping_context, swap_buffers());
    EXPECT_CALL(mock_gl, glUseProgram(0));

    glprogram.render(renderlist, offset, mock_swapping_context);
}

TEST_F(HWCFallbackGLRenderer, activates_alpha_per_renderable)
{
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubTranslucentRenderable>(),
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubTranslucentRenderable>()
    };

    mga::HWCFallbackGLRenderer glprogram(mock_gl_program_factory, mock_context, dummy_screen_pos);

    testing::InSequence seq;
    EXPECT_CALL(mock_gl, glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND));
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));

    glprogram.render(renderlist, offset, mock_swapping_context);
}
