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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/android/android_buffer_allocator.h"
#include "mir/graphics/android/android_display.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"

#include "../tools/mir_image.h"
#include <GLES2/gl2.h>
#include <stdexcept>
#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry; 
namespace mga=mir::graphics::android; 
namespace mg=mir::graphics; 

class AndroidBufferIntegration : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    }
};


TEST_F(AndroidBufferIntegration, alloc_does_not_throw)
{
    using namespace testing;

    EXPECT_NO_THROW({ 
    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);
    });

}

TEST_F(AndroidBufferIntegration, swapper_creation_ok)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);


    geom::Width  w(200);
    geom::Height h(400);
    mc::PixelFormat pf(mc::PixelFormat::rgba_8888);
    EXPECT_NO_THROW({ 
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf); 
    });
}

TEST_F(AndroidBufferIntegration, swapper_returns_non_null)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);


    geom::Width  w(200);
    geom::Height h(400);
    mc::PixelFormat pf(mc::PixelFormat::rgba_8888);
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf);

    EXPECT_NE((int)swapper->client_acquire(), NULL);
}

TEST_F(AndroidBufferIntegration, buffer_throws_with_no_egl_context)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);


    geom::Width  w(200);
    geom::Height h(400);
    mc::PixelFormat pf(mc::PixelFormat::rgba_8888);
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf);

    auto buffer = swapper->compositor_acquire();
    EXPECT_THROW({
    buffer->bind_to_texture();
    }, std::runtime_error);

}

namespace mg=mir::graphics;

static const GLchar *vtex_shader_src =
{
    "attribute vec4 vPosition;\n"
    "attribute vec4 uvCoord;\n"
    "varying vec2 texcoord;\n"
    "uniform float slide;\n"
    "void main() {\n"
    "   gl_Position = vPosition;\n"
    "   texcoord = uvCoord.xy + vec2(slide);\n"
    "}\n"
};

static const GLchar *frag_shader_src =
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, texcoord);\n"
    "}\n"
};

const GLint num_vertex = 4;
GLfloat vertex_data[] =
{
    -1.0f, -1.0f, 0.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    1.0f,  1.0f, 0.0f, 1.0f,
};

GLfloat uv_data[] =
{
    1.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f

};

void setup_gl(GLuint& program, GLuint& vPositionAttr, GLuint& uvCoord, GLuint& slideUniform)
{
    glClearColor(0.0, 1.0, 0.0, 1.0);

    GLuint vtex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtex_shader, 1, &vtex_shader_src, 0);
    glCompileShader(vtex_shader);

    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_shader_src, 0);
    glCompileShader(frag_shader);

    program = glCreateProgram();
    glAttachShader(program, vtex_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);

    vPositionAttr = glGetAttribLocation(program, "vPosition");
    glVertexAttribPointer(vPositionAttr, num_vertex, GL_FLOAT, GL_FALSE, 0, vertex_data);
    uvCoord = glGetAttribLocation(program, "uvCoord");
    glVertexAttribPointer(uvCoord, num_vertex, GL_FLOAT, GL_FALSE, 0, uv_data);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        mir_image.width, mir_image.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        mir_image.pixel_data);
    slideUniform = glGetUniformLocation(program, "slide");
}

void gl_render(GLuint program, GLuint vPosition, GLuint uvCoord,
               GLuint slideUniform, float slide)
{
    glUseProgram(program);

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    glUniform1fv(slideUniform, 1, &slide);

    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(uvCoord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertex);
    glDisableVertexAttribArray(uvCoord);
    glDisableVertexAttribArray(vPosition);
}

TEST_F(AndroidBufferIntegration, buffer_ok_with_egl_context)
{
    using namespace testing;

    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);


    geom::Width  w(200);
    geom::Height h(400);
    mc::PixelFormat pf(mc::PixelFormat::rgba_8888);
    std::unique_ptr<mc::BufferSwapper> swapper = strategy->create_swapper(w, h, pf);

    std::shared_ptr<mg::Display> display;
    display = mg::create_display();

    GLuint program, vPositionAttr, uvCoord, slideUniform;
    setup_gl(program, vPositionAttr, uvCoord, slideUniform);
    /* add to swapper to surface */
    /* add to surface to surface stack */

    /* add swapper to ipc mechanism thing */

    float slide =1.0;
    auto buffer = swapper->compositor_acquire();
    EXPECT_NO_THROW({
        buffer->bind_to_texture();
    });

    for(;;)
    {
        gl_render(program, vPositionAttr, uvCoord, slideUniform, slide);
        display->post_update();
    }
}



