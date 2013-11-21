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

#include "src/server/graphics/android/buffer.h"
#include "mir/graphics/egl_extensions.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_fence.h"
#include "mir_test_doubles/mock_android_native_buffer.h"

#include <system/window.h>
#include <stdexcept>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

class AndroidBufferBinding : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        mock_native_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();
        size = geom::Size{300, 220};
        pf = geom::PixelFormat::abgr_8888;
        extensions = std::make_shared<mg::EGLExtensions>();
    };

    geom::Size size;
    geom::PixelFormat pf;

    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mg::EGLExtensions> extensions;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> mock_native_buffer;
};

TEST_F(AndroidBufferBinding, buffer_queries_for_display)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_creates_image_on_first_bind)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_only_makes_one_image_per_display)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
    buffer.bind_to_texture();
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_anwb_is_bound)
{
    using namespace testing;
    ANativeWindowBuffer *stub_anwb = reinterpret_cast<ANativeWindowBuffer*>(0xdeed);
    EXPECT_CALL(*mock_native_buffer, anwb())
        .Times(1)
        .WillOnce(Return(stub_anwb));
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,stub_anwb,_))
        .Times(Exactly(1));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_makes_new_image_with_new_display)
{
    using namespace testing;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)mock_egl.fake_egl_display +1);

    /* return 1st fake display */
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(2));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();

    /* return 2nd fake display */
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(second_fake_display));

    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_frees_images_it_makes)
{
    using namespace testing;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)mock_egl.fake_egl_display +1);

    EXPECT_CALL(mock_egl, eglDestroyImageKHR(_,_))
        .Times(Exactly(2));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();

    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(second_fake_display));

    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_frees_images_it_makes_with_proper_args)
{
    using namespace testing;

    EGLDisplay first_fake_display = mock_egl.fake_egl_display;
    EGLImageKHR first_fake_egl_image = (EGLImageKHR) 0x84210;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)mock_egl.fake_egl_display +1);
    EGLImageKHR second_fake_egl_image = (EGLImageKHR) 0x84211;

    /* actual expectations */
    EXPECT_CALL(mock_egl, eglDestroyImageKHR(first_fake_display, first_fake_egl_image))
        .Times(Exactly(1));
    EXPECT_CALL(mock_egl, eglDestroyImageKHR(second_fake_display, second_fake_egl_image))
        .Times(Exactly(1));

    /* manipulate mock to return 1st set */
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(first_fake_display));
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((first_fake_egl_image)));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();

    /* manipulate mock to return 2nd set */
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(second_fake_display));
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((second_fake_egl_image)));

    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_uses_current_display)
{
    using namespace testing;
    EGLDisplay fake_display = (EGLDisplay) 0x32298;

    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(fake_display));

    EXPECT_CALL(mock_egl, eglCreateImageKHR(fake_display,_,_,_,_))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_specifies_no_context)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateImageKHR(_, EGL_NO_CONTEXT,_,_,_))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_sets_egl_native_buffer_android)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,EGL_NATIVE_BUFFER_ANDROID,_,_))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_sets_proper_attributes)
{
    using namespace testing;

    const EGLint* attrs;

    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(DoAll(SaveArg<4>(&attrs),
                        Return(mock_egl.fake_egl_image)));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();

    /* note: this should not segfault. if it does, the attributes were set wrong */
    EXPECT_EQ(attrs[0],    EGL_IMAGE_PRESERVED_KHR);
    EXPECT_EQ(attrs[1],    EGL_TRUE);
    EXPECT_EQ(attrs[2],    EGL_NONE);

}

TEST_F(AndroidBufferBinding, buffer_destroys_correct_buffer_with_single_image)
{
    using namespace testing;
    EGLImageKHR fake_egl_image = (EGLImageKHR) 0x84210;
    EXPECT_CALL(mock_egl, eglCreateImageKHR(mock_egl.fake_egl_display,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((fake_egl_image)));
    EXPECT_CALL(mock_egl, eglDestroyImageKHR(mock_egl.fake_egl_display, fake_egl_image))
        .Times(Exactly(1));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_image_creation_failure_does_not_save)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(2))
        .WillRepeatedly(Return((EGL_NO_IMAGE_KHR)));
    EXPECT_CALL(mock_egl, eglDestroyImageKHR(_,_))
        .Times(Exactly(0));

    mga::Buffer buffer(mock_native_buffer, extensions);
    EXPECT_THROW(
    {
        buffer.bind_to_texture();
    }, std::runtime_error);

    EXPECT_THROW(
    {
        buffer.bind_to_texture();
    }, std::runtime_error);
}

TEST_F(AndroidBufferBinding, buffer_image_creation_failure_throws)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillRepeatedly(Return((EGL_NO_IMAGE_KHR)));

    mga::Buffer buffer(mock_native_buffer, extensions);
    EXPECT_THROW(
    {
        buffer.bind_to_texture();
    }, std::runtime_error);
}


/* binding tests */
TEST_F(AndroidBufferBinding, buffer_calls_binding_extension)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_, _))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_calls_binding_extension_every_time)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_, _))
        .Times(Exactly(3));

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
    buffer.bind_to_texture();
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_binding_specifies_gl_texture_2d)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, _))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_binding_uses_right_image)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_, mock_egl.fake_egl_image))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, buffer_binding_uses_right_image_after_display_swap)
{
    using namespace testing;
    EGLDisplay second_fake_display = (EGLDisplay) ((int)mock_egl.fake_egl_display +1);
    EGLImageKHR second_fake_egl_image = (EGLImageKHR) 0x84211;

    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_, _))
        .Times(Exactly(1));
    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();

    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_, second_fake_egl_image))
        .Times(Exactly(1));
    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(Exactly(1))
        .WillOnce(Return(second_fake_display));
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .Times(Exactly(1))
        .WillOnce(Return((second_fake_egl_image)));
    buffer.bind_to_texture();
}

TEST_F(AndroidBufferBinding, bind_to_texture_waits_on_fence)
{
    using namespace testing;
    EXPECT_CALL(*mock_native_buffer, wait_for_content())
        .Times(1);

    mga::Buffer buffer(mock_native_buffer, extensions);
    buffer.bind_to_texture();
}
