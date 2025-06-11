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

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/rectangle.h>
#include <mir/test/fake_shared.h>
#include <mir/test/doubles/mock_gl_buffer.h>
#include <mir/test/doubles/mock_renderable.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/test/doubles/mock_gl.h>
#include <mir/test/doubles/mock_egl.h>
#include <src/renderers/gl/renderer.h>
#include <mir/test/doubles/stub_gl_rendering_provider.h>
#include <mir/test/doubles/mock_output_surface.h>

#include "mir/graphics/transformation.h"

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::ReturnRef;
using testing::Pointee;
using testing::AnyNumber;
using testing::AtLeast;
using testing::DoAll;
using testing::_;

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mg=mir::graphics;
namespace mgl=mir::gl;
namespace mrg = mir::renderer::gl;

namespace
{
const GLint stub_v_shader = 1;
const GLint stub_f_shader = 2;
const GLint stub_program = 1;
const GLint transform_uniform_location = 1;
const GLint alpha_uniform_location = 2;
const GLint position_attr_location = 3;
const GLint texcoord_attr_location = 4;
const GLint screen_to_gl_coords_uniform_location = 5;
const GLint tex_uniform_location = 6;
const GLint display_transform_uniform_location = 7;
const GLint centre_uniform_location = 8;

void SetUpMockProgramData(mtd::MockGL &mock_gl)
{
    /* Uniforms and Attributes */
    ON_CALL(mock_gl, glGetAttribLocation(stub_program, "position"))
        .WillByDefault(Return(position_attr_location));
    ON_CALL(mock_gl, glGetAttribLocation(stub_program, "texcoord"))
        .WillByDefault(Return(texcoord_attr_location));

    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "tex"))
        .WillByDefault(Return(tex_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "tex1"))
        .WillByDefault(Return(-1));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "tex2"))
        .WillByDefault(Return(-1));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "centre"))
        .WillByDefault(Return(centre_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "display_transform"))
        .WillByDefault(Return(display_transform_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "transform"))
        .WillByDefault(Return(transform_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "screen_to_gl_coords"))
        .WillByDefault(Return(screen_to_gl_coords_uniform_location));
    ON_CALL(mock_gl, glGetUniformLocation(stub_program, "alpha"))
        .WillByDefault(Return(alpha_uniform_location));
}

class GLRenderer :
    public testing::Test
{
public:

    GLRenderer()
    {
        //Mock defaults
        ON_CALL(mock_gl, glCreateShader(GL_VERTEX_SHADER))
            .WillByDefault(Return(stub_v_shader));
        ON_CALL(mock_gl, glCreateShader(GL_FRAGMENT_SHADER))
            .WillByDefault(Return(stub_f_shader));
        ON_CALL(mock_gl, glCreateProgram())
            .WillByDefault(Return(stub_program));
        ON_CALL(mock_gl, glGetProgramiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetShaderiv(_,_,_))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));

        //A mix of defaults and silencing from here on out
        EXPECT_CALL(mock_gl, glUseProgram(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glActiveTexture(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniformMatrix4fv(_, _, GL_FALSE, _))
            .Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform1f(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform2f(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glBindBuffer(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glVertexAttribPointer(_, _, _, _, _, _))
            .Times(AnyNumber());
        EXPECT_CALL(mock_gl, glEnableVertexAttribArray(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDisableVertexAttribArray(_)).Times(AnyNumber());

        mock_buffer = std::make_shared<testing::NiceMock<mtd::MockTextureBuffer>>();
        EXPECT_CALL(*mock_buffer, id())
            .WillRepeatedly(Return(mir::graphics::BufferID(789)));
        EXPECT_CALL(*mock_buffer, layout())
            .WillRepeatedly(Return(mir::graphics::gl::Texture::Layout::GL));
        EXPECT_CALL(*mock_buffer, size())
            .WillRepeatedly(Return(mir::geometry::Size{123, 456}));
        ON_CALL(*mock_buffer, shader(_))
            .WillByDefault(testing::Invoke(
                [](auto& factory) -> mg::gl::Program&
                {
                    static int unused = 1;
                    return factory.compile_fragment_shader(&unused, "extension code", "fragment code");
                }));

        renderable = std::make_shared<testing::NiceMock<mtd::MockRenderable>>();
        EXPECT_CALL(*renderable, id()).WillRepeatedly(Return(&renderable));
        EXPECT_CALL(*renderable, buffer()).WillRepeatedly(Return(mock_buffer));
        EXPECT_CALL(*renderable, shaped()).WillRepeatedly(Return(false));
        EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(1.0f));
        EXPECT_CALL(*renderable, transformation()).WillRepeatedly(Return(trans));
        EXPECT_CALL(*renderable, orientation()).WillRepeatedly(Return(mir_orientation_normal));
        EXPECT_CALL(*renderable, screen_position())
            .WillRepeatedly(Return(mir::geometry::Rectangle{{1,2},{3,4}}));
        EXPECT_CALL(*renderable, clip_area())
            .WillRepeatedly(Return(std::optional<mir::geometry::Rectangle>()));
        EXPECT_CALL(mock_gl, glDisable(_)).Times(AnyNumber());

        renderable_list.push_back(renderable);

        InSequence s;
        SetUpMockProgramData(mock_gl);

        EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
            .WillRepeatedly(Return(screen_to_gl_coords_uniform_location));
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockTextureBuffer> mock_buffer;
    std::shared_ptr<testing::NiceMock<mtd::MockRenderable>> renderable;
    mg::RenderableList renderable_list;
    glm::mat4 trans{1.f};
    std::shared_ptr<mtd::StubGlRenderingProvider> const gl_platform{std::make_shared<mtd::StubGlRenderingProvider>()};

    class StubProgram : public mg::gl::Program
    {
    };
    StubProgram prog;
};

auto make_output_surface() -> std::unique_ptr<mtd::MockOutputSurface>
{
    return std::make_unique<testing::NiceMock<mtd::MockOutputSurface>>();
}

}

TEST_F(GLRenderer, disables_blending_for_rgbx_surfaces)
{
    InSequence seq;
    EXPECT_CALL(*renderable, shaped())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND));

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, enables_blending_for_rgba_surfaces)
{
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(true));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, enables_blending_for_rgbx_translucent_surfaces)
{
    EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(0.5f));
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, uses_premultiplied_src_alpha_for_rgba_surfaces)
{
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(true));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl, glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                                             GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, avoids_src_alpha_for_rgbx_blending)  // LP: #1423462
{
    EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(0.5f));
    EXPECT_CALL(*renderable, shaped()).WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND)).Times(0);
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl,
                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_CONSTANT_ALPHA,
                                    GL_ZERO, GL_ONE));

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, applies_inverse_orientation_matrix)
{
    InSequence seq;
    EXPECT_CALL(mock_gl, glUniformMatrix4fv(_, _, _, _))
        .Times(2); // Display transform
    EXPECT_CALL(*renderable, orientation())
        .WillOnce(Return(mir_orientation_left));
    EXPECT_CALL(mock_gl, glUniformMatrix4fv(
        _, _, _, testing::Truly([trans=trans](float const* ptr)
        {
            glm::mat4 mat;
            for (size_t i = 0; i < 4; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    mat[i][j] = ptr[i * 4 + j];
                }
            }
            glm::mat4 const expected = trans
                * glm::mat4(mir::graphics::inverse_transformation(mir_orientation_left));
            for (size_t i = 0; i < 4; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    printf("%f, %f\n", mat[i][j], expected[i][j]);
                }
            }

            return mat == expected;
        }))).Times(1);

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, clears_to_opaque_black)
{
    InSequence seq;
    EXPECT_CALL(mock_gl, glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_CALL(mock_gl, glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    EXPECT_CALL(mock_gl, glClear(_));

    mrg::Renderer renderer(gl_platform, make_output_surface());

    renderer.render(renderable_list);
}

TEST_F(GLRenderer, makes_display_buffer_current_when_created)
{
    auto mock_output_surface = make_output_surface();

    EXPECT_CALL(*mock_output_surface, make_current());

    mrg::Renderer renderer(gl_platform, std::move(mock_output_surface));
}

TEST_F(GLRenderer, makes_display_buffer_current_before_rendering)
{
    auto mock_output_surface = make_output_surface();

    InSequence seq;
    EXPECT_CALL(*mock_output_surface, make_current()).Times(AnyNumber());
    EXPECT_CALL(mock_gl, glClear(_));

    mrg::Renderer renderer(gl_platform, std::move(mock_output_surface));

    renderer.render(renderable_list);
}

TEST_F(GLRenderer, swaps_buffers_after_rendering)
{
    auto mock_output_surface = make_output_surface();

    InSequence seq;
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_output_surface, commit())
        .WillRepeatedly(testing::Invoke([]() { return std::unique_ptr<mg::Framebuffer>(); }));

    mrg::Renderer renderer(gl_platform, std::move(mock_output_surface));
    renderer.render(renderable_list);
}

TEST_F(GLRenderer, sets_scissor_test)
{
    EXPECT_CALL(*renderable, clip_area())
        .WillRepeatedly(Return(std::optional<mir::geometry::Rectangle>({{0,1},{2,3}})));
    EXPECT_CALL(mock_gl, glEnable(GL_SCISSOR_TEST));
    EXPECT_CALL(mock_gl, glDisable(GL_SCISSOR_TEST));
    EXPECT_CALL(mock_gl, glScissor(-1, 2, 2, 3));

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.set_viewport({{1, 2}, {3, 4}});

    renderer.render(renderable_list);
}

TEST_F(GLRenderer, dont_set_scissor_test_when_unnecessary)
{
    EXPECT_CALL(mock_gl, glEnable(GL_SCISSOR_TEST)).Times(0);
    EXPECT_CALL(mock_gl, glDisable(GL_SCISSOR_TEST)).Times(0);
    EXPECT_CALL(mock_gl, glScissor(_, _, _, _)).Times(0);

    mrg::Renderer renderer(gl_platform, make_output_surface());
    renderer.set_viewport(mir::geometry::Rectangle{{0, 0}, {2, 3}});

    renderer.render(renderable_list);
}


TEST_F(GLRenderer, unchanged_viewport_avoids_gl_calls)
{
    mir::geometry::Rectangle const view_area{{0,0}, {1920,1080}};

    mrg::Renderer renderer(gl_platform, make_output_surface());

    renderer.set_viewport(view_area);

    EXPECT_CALL(mock_gl, glViewport(_,_,_,_))
        .Times(0);
    renderer.set_viewport(view_area);
}

TEST_F(GLRenderer, unchanged_viewport_updates_gl_if_rotated)
{   // Regression test for LP: #1672269
    int const screen_width = 1920;
    int const screen_height = 1080;
    mir::geometry::Rectangle const view_area{{0,0}, {1920,1080}};

    auto output_surface = make_output_surface();

    ON_CALL(*output_surface, size())
        .WillByDefault(
            Return(
                mir::geometry::Size{screen_width, screen_height}));

    mrg::Renderer renderer(gl_platform, std::move(output_surface));

    renderer.set_viewport(view_area);

    EXPECT_CALL(mock_gl, glViewport(_,_,_,_))
        .Times(1);
    glm::mat2 const something_different{0,-1,
                                        1, 0};
    renderer.set_output_transform(something_different);
    renderer.set_viewport(view_area);
}

TEST_F(GLRenderer, sets_viewport_unscaled_exact)
{
    int const screen_width = 1920;
    int const screen_height = 1080;
    mir::geometry::Rectangle const view_area{{0,0}, {1920,1080}};

    auto output_surface = make_output_surface();

    ON_CALL(*output_surface, size())
        .WillByDefault(
            Return(
                mir::geometry::Size{screen_width, screen_height}));

    EXPECT_CALL(mock_gl, glViewport(0, 0, screen_width, screen_height));

    mrg::Renderer renderer(gl_platform, std::move(output_surface));
    renderer.set_viewport(view_area);
}

TEST_F(GLRenderer, sets_viewport_upscaled_exact)
{
    int const screen_width = 1920;
    int const screen_height = 1080;
    mir::geometry::Rectangle const view_area{{0,0}, {1280,720}};

    auto output_surface = make_output_surface();

    ON_CALL(*output_surface, size())
        .WillByDefault(
            Return(
                mir::geometry::Size{screen_width, screen_height}));

    EXPECT_CALL(mock_gl, glViewport(0, 0, screen_width, screen_height));

    mrg::Renderer renderer(gl_platform, std::move(output_surface));
    renderer.set_viewport(view_area);
}

TEST_F(GLRenderer, sets_viewport_downscaled_exact)
{
    int const screen_width = 1280;
    int const screen_height = 720;
    mir::geometry::Rectangle const view_area{{0,0}, {1920,1080}};

    auto output_surface = make_output_surface();

    ON_CALL(*output_surface, size())
        .WillByDefault(
            Return(
                mir::geometry::Size{screen_width, screen_height}));

    EXPECT_CALL(mock_gl, glViewport(0, 0, screen_width, screen_height));

    mrg::Renderer renderer(gl_platform, std::move(output_surface));
    renderer.set_viewport(view_area);
}

TEST_F(GLRenderer, sets_viewport_upscaled_narrow)
{
    int const screen_width = 1920;
    int const screen_height = 1080;
    mir::geometry::Rectangle const view_area{{0,0}, {640,480}};

    auto output_surface = make_output_surface();

    ON_CALL(*output_surface, size())
        .WillByDefault(
            Return(
                mir::geometry::Size{screen_width, screen_height}));

    EXPECT_CALL(mock_gl, glViewport(240, 0, 1440, 1080));

    mrg::Renderer renderer(gl_platform, std::move(output_surface));
    renderer.set_viewport(view_area);
}

TEST_F(GLRenderer, sets_viewport_downscaled_wide)
{
    int const screen_width = 640;
    int const screen_height = 480;
    mir::geometry::Rectangle const view_area{{0,0}, {1920,1080}};

    auto output_surface = make_output_surface();

    ON_CALL(*output_surface, size())
        .WillByDefault(
            Return(
                mir::geometry::Size{screen_width, screen_height}));

    EXPECT_CALL(mock_gl, glViewport(0, 60, 640, 360));

    mrg::Renderer renderer(gl_platform, std::move(output_surface));
    renderer.set_viewport(view_area);
}
