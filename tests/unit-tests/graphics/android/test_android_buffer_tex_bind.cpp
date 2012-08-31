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

#include "mir_test/mock_alloc_adaptor.h"
#include "mir_test/gl_mock.h"
#include "mir_test/egl_mock.h"

#include <gtest/gtest.h>

namespace mg =mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;
namespace mc=mir::compositor;

class AndroidBufferBinding : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;
        
        std::shared_ptr<mg::MockAllocAdaptor> mock_alloc_dev(new mg::MockAllocAdaptor);
        EXPECT_CALL(*mock_alloc_dev, alloc_buffer( _, _, _, _, _, _))
            .Times(AtLeast(0));

        buffer = std::shared_ptr<mc::Buffer>(
                            new mga::AndroidBuffer(mock_alloc_dev, geom::Width(300), geom::Height(200), mc::PixelFormat::rgba_8888));
    };
    virtual void TearDown()
    {
        buffer.reset();
    };
    std::shared_ptr<mc::Buffer> buffer;
    mir::GLMock gl_mock;
    mir::EglMock egl_mock;
};

TEST_F(AndroidBufferBinding, buffer_queries_for_display)
{
    using namespace testing;
    EXPECT_CALL(egl_mock, eglGetCurrentDisplay())
            .Times(Exactly(1));

    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_creates_image_on_first_bind)
{
    using namespace testing;
    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1));
    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_only_makes_one_image_per_display)
{
    using namespace testing;
    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1));

    buffer->bind_to_texture();
    buffer->bind_to_texture();
    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_makes_new_image_with_new_display)
{
    using namespace testing;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)egl_mock.fake_egl_display +1);

    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(2));

    buffer->bind_to_texture();

    EXPECT_CALL(egl_mock, eglGetCurrentDisplay())
            .Times(Exactly(1))
            .WillOnce(Return(second_fake_display));

    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_frees_images_it_makes)
{
    using namespace testing;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)egl_mock.fake_egl_display +1);

    EXPECT_CALL(egl_mock, eglDestroyImageKHR(_,_))
        .Times(Exactly(2));

    buffer->bind_to_texture();

    EXPECT_CALL(egl_mock, eglGetCurrentDisplay())
            .Times(Exactly(1))
            .WillOnce(Return(second_fake_display));

    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_frees_images_it_makes_with_proper_args)
{
    using namespace testing;

    EGLDisplay first_fake_display = egl_mock.fake_egl_display;
    EGLImageKHR first_fake_egl_image = (EGLImageKHR) 0x84210;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)egl_mock.fake_egl_display +1);
    EGLImageKHR second_fake_egl_image = (EGLImageKHR) 0x84211;

    /* actual expectations */
    EXPECT_CALL(egl_mock, eglDestroyImageKHR(first_fake_display, first_fake_egl_image))
        .Times(Exactly(1));
    EXPECT_CALL(egl_mock, eglDestroyImageKHR(second_fake_display, second_fake_egl_image))
        .Times(Exactly(1));

    /* manipulate mock to return 1st set */
    EXPECT_CALL(egl_mock, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(first_fake_display));
    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((first_fake_egl_image)));

    buffer->bind_to_texture();

    /* manipulate mock to return 2nd set */
    EXPECT_CALL(egl_mock, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(second_fake_display));
    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((second_fake_egl_image)));

    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_uses_current_display)
{
    using namespace testing;
    EGLDisplay fake_display = (EGLDisplay) 0x32298;    

    EXPECT_CALL(egl_mock, eglGetCurrentDisplay())
            .Times(Exactly(1))
            .WillOnce(Return(fake_display));

    EXPECT_CALL(egl_mock, eglCreateImageKHR(fake_display,_,_,_,_))
        .Times(Exactly(1));
    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_specifies_no_context)
{
    using namespace testing;

    EXPECT_CALL(egl_mock, eglCreateImageKHR(_, EGL_NO_CONTEXT,_,_,_))
        .Times(Exactly(1));
    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_sets_egl_native_buffer_android)
{
    using namespace testing;

    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,EGL_NATIVE_BUFFER_ANDROID,_,_))
        .Times(Exactly(1));
    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_destroys_correct_buffer_with_single_image)
{
    using namespace testing;
    EGLImageKHR fake_egl_image = (EGLImageKHR) 0x84210;
    EXPECT_CALL(egl_mock, eglCreateImageKHR(egl_mock.fake_egl_display,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((fake_egl_image)));
    EXPECT_CALL(egl_mock, eglDestroyImageKHR(egl_mock.fake_egl_display, fake_egl_image))
        .Times(Exactly(1));

    buffer->bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_image_creation_failure_does_not_save)
{
    using namespace testing;
    EXPECT_CALL(egl_mock, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(2))
        .WillRepeatedly(Return((EGL_NO_IMAGE_KHR)));
    EXPECT_CALL(egl_mock, eglDestroyImageKHR(_,_))
        .Times(Exactly(0));

    buffer->bind_to_texture();
    buffer->bind_to_texture();
}
