/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <functional>
#include <string>
#include <cstring>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/rectangle.h>
#include "mir/compositor/renderer.h"
#include "src/server/compositor/gl_renderer_factory.h"
#include "src/server/graphics/program_factory.h"
#include <mir_test/fake_shared.h>
#include <mir_test_doubles/mock_buffer.h>
#include <mir_test_doubles/mock_renderable.h>
#include <mir_test_doubles/mock_buffer_stream.h>
#include <mir/compositor/buffer_stream.h>
#include <mir_test_doubles/mock_gl.h>

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::ReturnRef;
using testing::Pointee;
using testing::AnyNumber;
using testing::AtLeast;
using testing::_;

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mg=mir::graphics;

namespace
{

const GLint stub_v_shader = 1;
const GLint stub_f_shader = 2;
const GLint stub_program = 1;
const GLuint stub_texture = 1;
const GLint transform_uniform_location = 1;
const GLint alpha_uniform_location = 2;
const GLint position_attr_location = 3;
const GLint texcoord_attr_location = 4;
const GLint screen_to_gl_coords_uniform_location = 5;
const GLint tex_uniform_location = 6;
const GLint display_transform_uniform_location = 7;
const GLint centre_uniform_location = 8;
const std::string stub_info_log = "something failed!";
const size_t stub_info_log_length = stub_info_log.size();


void SetUpMockProgramData(mtd::MockGL &mock_gl)
{
    /* Uniforms and Attributes */
    EXPECT_CALL(mock_gl, glUseProgram(stub_program));

    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(tex_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(display_transform_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(transform_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(alpha_uniform_location));
    EXPECT_CALL(mock_gl, glGetAttribLocation(stub_program, _))
        .WillOnce(Return(position_attr_location));
    EXPECT_CALL(mock_gl, glGetAttribLocation(stub_program, _))
        .WillOnce(Return(texcoord_attr_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(centre_uniform_location));
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
            .Times(AtLeast(1));
        EXPECT_CALL(mock_gl, glUniform1f(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform2f(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glBindBuffer(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glVertexAttribPointer(_, _, _, _, _, _))
            .Times(AnyNumber());
        EXPECT_CALL(mock_gl, glEnableVertexAttribArray(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDisableVertexAttribArray(_)).Times(AnyNumber());

        mock_buffer = std::make_shared<mtd::MockBuffer>();
        EXPECT_CALL(*mock_buffer, bind_to_texture()).Times(AnyNumber());
        EXPECT_CALL(*mock_buffer, size())
            .WillRepeatedly(Return(mir::geometry::Size{123, 456}));

        renderable = std::make_shared<testing::NiceMock<mtd::MockRenderable>>();
        EXPECT_CALL(*renderable, id()).WillRepeatedly(Return(&renderable));
        EXPECT_CALL(*renderable, buffer()).WillRepeatedly(Return(mock_buffer));
        EXPECT_CALL(*renderable, shaped()).WillRepeatedly(Return(false));
        EXPECT_CALL(*renderable, alpha()).WillRepeatedly(Return(1.0f));
        EXPECT_CALL(*renderable, transformation()).WillRepeatedly(Return(trans));
        EXPECT_CALL(*renderable, screen_position())
            .WillRepeatedly(Return(mir::geometry::Rectangle{{1,2},{3,4}}));
        EXPECT_CALL(mock_gl, glDisable(_)).Times(AnyNumber());

        renderable_list.push_back(renderable);

        InSequence s;
        SetUpMockProgramData(mock_gl);
        EXPECT_CALL(mock_gl, glUniform1i(tex_uniform_location, 0));

        EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
            .WillOnce(Return(screen_to_gl_coords_uniform_location));

        mc::GLRendererFactory gl_renderer_factory{std::make_shared<mg::ProgramFactory>()};
        display_area = {{1, 2}, {3, 4}};
        renderer = gl_renderer_factory.create_renderer_for(display_area);
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    mir::geometry::Rectangle display_area;
    std::unique_ptr<mc::Renderer> renderer;
    std::shared_ptr<testing::NiceMock<mtd::MockRenderable>> renderable;
    mg::RenderableList renderable_list;
    glm::mat4 trans;
};

}

TEST_F(GLRenderer, TestSetUpRenderContextBeforeRendering)
{
    using namespace std::placeholders;

    InSequence seq;

    EXPECT_CALL(mock_gl, glClearColor(_, _, _, 1.0f));
    EXPECT_CALL(mock_gl, glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    EXPECT_CALL(mock_gl, glClear(_));
    EXPECT_CALL(mock_gl, glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE));
    EXPECT_CALL(mock_gl, glUseProgram(stub_program));
    EXPECT_CALL(*renderable, shaped())
        .WillOnce(Return(true));
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl, glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    EXPECT_CALL(mock_gl, glActiveTexture(GL_TEXTURE0));

    EXPECT_CALL(mock_gl, glUniform2f(centre_uniform_location, _, _));
    EXPECT_CALL(*renderable, transformation())
        .WillOnce(Return(trans));
    EXPECT_CALL(mock_gl, glUniformMatrix4fv(transform_uniform_location, 1, GL_FALSE, _));
    EXPECT_CALL(*renderable, alpha())
        .WillOnce(Return(0.0f));
    EXPECT_CALL(mock_gl, glUniform1f(alpha_uniform_location, _));

    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _));

    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(position_attr_location));
    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(texcoord_attr_location));

    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_location, 3,
                                               GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(texcoord_attr_location, 2,
                                               GL_FLOAT, GL_FALSE, _, _));

    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(texcoord_attr_location));
    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(position_attr_location));

    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)));

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();
}

TEST_F(GLRenderer, disables_blending_for_rgbx_surfaces)
{
    InSequence seq;
    EXPECT_CALL(*renderable, shaped())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND));

    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _));

    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)));

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();
}

TEST_F(GLRenderer, caches_and_uploads_texture_only_on_buffer_changes)
{
    InSequence seq;

    // First render() - texture generated and uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                         _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                         _));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(_, _))
        .Times(0);

    // Second render() - texture found in cache and not re-uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(_, _))
        .Times(0);

    // Third render() - texture found in cache but refreshed with new buffer
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)))
        .Times(0);

    // Forth render() - stale texture reuploaded following bypass
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)))
        .Times(0);

    // Fifth render() - texture found in cache and not re-uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(_, _))
        .Times(0);

    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)));

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();

    renderer->suspend();

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();

    renderer->begin();
    renderer->render(renderable_list);
    renderer->end();
}

TEST_F(GLRenderer, holds_buffers_till_the_end)
{
    InSequence seq;

    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                         _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                         _));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));

    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)));

    long old_use_count = mock_buffer.use_count();

    renderer->begin();
    renderer->render(renderable_list);
    EXPECT_EQ(old_use_count+1, mock_buffer.use_count());
    renderer->end();
    EXPECT_EQ(old_use_count, mock_buffer.use_count());
}
