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

#include "src/server/graphics/android/android_display.h"
#include "src/server/graphics/android/hwc_display.h"
#include "mir_test_doubles/mock_android_framebuffer_window.h"
#include "mir_test_doubles/mock_hwc_interface.h"
#include "mir_test/egl_mock.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

template<class T>
std::shared_ptr<mg::DisplayBuffer> make_display_buffer(std::shared_ptr<mtd::MockAndroidFramebufferWindow> const& fbwin);

template <>
std::shared_ptr<mg::DisplayBuffer> make_display_buffer<mga::AndroidDisplay>(
                                std::shared_ptr<mtd::MockAndroidFramebufferWindow> const& fbwin)
{
    return std::make_shared<mga::AndroidDisplay>(fbwin);
}

template <>
std::shared_ptr<mg::DisplayBuffer> make_display_buffer<mga::HWCDisplay>(
                                std::shared_ptr<mtd::MockAndroidFramebufferWindow> const& fbwin)
{
    auto mock_hwc_device = std::make_shared<mtd::MockHWCInterface>();
    return std::make_shared<mga::HWCDisplay>(fbwin, mock_hwc_device);
}

template<typename T>
class AndroidTestFramebufferInit : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        native_win = std::make_shared<NiceMock<mtd::MockAndroidFramebufferWindow>>();

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();

        EXPECT_CALL(*native_win, android_native_window_type())
        .Times(AtLeast(0));
        EXPECT_CALL(*native_win, android_display_egl_config(_))
        .Times(AtLeast(0));

        width = 435;
        height = 477;
    }

    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    mir::EglMock mock_egl;
    int width, height;
};

typedef ::testing::Types<mga::AndroidDisplay, mga::HWCDisplay> FramebufferTestTypes;
TYPED_TEST_CASE(AndroidTestFramebufferInit, FramebufferTestTypes);

TYPED_TEST(AndroidTestFramebufferInit, eglGetDisplay)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, eglGetDisplay_failure)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
    .Times(Exactly(1))
    .WillOnce(Return((EGLDisplay)EGL_NO_DISPLAY));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error   );
}

TYPED_TEST(AndroidTestFramebufferInit, eglInitialize)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglInitialize(this->mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, eglInitialize_failure)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglInitialize(this->mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error   );
}

TYPED_TEST(AndroidTestFramebufferInit, eglInitialize_failure_bad_major_version)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglInitialize(this->mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SetArgPointee<1>(2),
                  Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error   );
}

TYPED_TEST(AndroidTestFramebufferInit, eglInitialize_failure_bad_minor_version)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglInitialize(this->mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SetArgPointee<2>(2),
                  Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error   );
}

TYPED_TEST(AndroidTestFramebufferInit, eglCreateWindowSurface_requests_config)
{
    using namespace testing;
    EGLConfig fake_config = (EGLConfig) 0x3432;
    EXPECT_CALL(*this->native_win, android_display_egl_config(_))
    .Times(Exactly(1))
    .WillOnce(Return(fake_config));
    EXPECT_CALL(this->mock_egl, eglCreateWindowSurface(this->mock_egl.fake_egl_display, fake_config, _, _))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, eglCreateWindowSurface_nullarg)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglCreateWindowSurface(this->mock_egl.fake_egl_display, _, _, NULL))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, eglCreateWindowSurface_uses_native_window_type)
{
    using namespace testing;
    EGLNativeWindowType egl_window = (EGLNativeWindowType)0x4443;

    EXPECT_CALL(*this->native_win, android_native_window_type())
    .Times(Exactly(1))
    .WillOnce(Return(egl_window));
    EXPECT_CALL(this->mock_egl, eglCreateWindowSurface(this->mock_egl.fake_egl_display, _, egl_window,_))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, eglCreateWindowSurface_failure)
{
    using namespace testing;
    EXPECT_CALL(this->mock_egl, eglCreateWindowSurface(this->mock_egl.fake_egl_display,_,_,_))
    .Times(Exactly(1))
    .WillOnce(Return((EGLSurface)NULL));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error);
}

/* create context stuff */
TYPED_TEST(AndroidTestFramebufferInit, CreateContext_window_cfg_matches_context_cfg)
{
    using namespace testing;

    EGLConfig chosen_cfg, cfg;
    EXPECT_CALL(this->mock_egl, eglCreateWindowSurface(this->mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SaveArg<1>(&chosen_cfg),
                  Return((EGLSurface)this->mock_egl.fake_egl_surface)));

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, Ne(EGL_NO_CONTEXT),_))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SaveArg<1>(&cfg),
                  Return((EGLContext)this->mock_egl.fake_egl_context)));

    auto display = make_display_buffer<TypeParam>(this->native_win);

    EXPECT_EQ(chosen_cfg, cfg);
}

TYPED_TEST(AndroidTestFramebufferInit, CreateContext_contexts_are_shared)
{
    using namespace testing;

    EGLContext const shared_ctx{reinterpret_cast<EGLContext>(0x17)};

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, EGL_NO_CONTEXT,_))
    .Times(Exactly(1))
    .WillOnce(Return(shared_ctx));

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, shared_ctx,_))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

namespace
{

ACTION_P(AppendContextAttrPtr, vec)
{
    vec->push_back(arg3);
}

}

TYPED_TEST(AndroidTestFramebufferInit, CreateContext_context_attr_null_terminated)
{
    using namespace testing;

    std::vector<EGLint const*> context_attr_ptrs;

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, _, _ ))
    .Times(AtLeast(2))
    .WillRepeatedly(DoAll(AppendContextAttrPtr(&context_attr_ptrs),
                          Return((EGLContext)this->mock_egl.fake_egl_context)));

    auto display = make_display_buffer<TypeParam>(this->native_win);

    for (auto context_attr : context_attr_ptrs)
    {
        int i = 0;
        ASSERT_NE(nullptr, context_attr);
        while(context_attr[i++] != EGL_NONE);
    }
}

TYPED_TEST(AndroidTestFramebufferInit, CreateContext_context_uses_client_version_2)
{
    using namespace testing;

    std::vector<EGLint const*> context_attr_ptrs;

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, _, _ ))
    .Times(AtLeast(2))
    .WillRepeatedly(DoAll(AppendContextAttrPtr(&context_attr_ptrs),
                          Return((EGLContext)this->mock_egl.fake_egl_context)));

    auto display = make_display_buffer<TypeParam>(this->native_win);

    for (auto context_attr : context_attr_ptrs)
    {
        int i = 0;
        bool validated = false;
        while(context_attr[i] != EGL_NONE)
        {
            if ((context_attr[i] == EGL_CONTEXT_CLIENT_VERSION) && (context_attr[i+1] == 2))
            {
                validated = true;
                break;
            }
            i++;
        }

        auto iter = std::find(context_attr_ptrs.begin(), context_attr_ptrs.end(),
                              context_attr);
        EXPECT_TRUE(validated) << " at index " << iter - context_attr_ptrs.begin();
    };
}

TYPED_TEST(AndroidTestFramebufferInit, CreateContext_failure)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, _, _ ))
    .Times(Exactly(1))
    .WillOnce(Return((EGLContext)EGL_NO_CONTEXT));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error   );
}

TYPED_TEST(AndroidTestFramebufferInit, MakeCurrent_uses_correct_pbuffer_surface)
{
    using namespace testing;
    EGLSurface fake_surface = (EGLSurface) 0x715;

    EXPECT_CALL(this->mock_egl, eglCreatePbufferSurface(this->mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(fake_surface));
    EXPECT_CALL(this->mock_egl, eglMakeCurrent(this->mock_egl.fake_egl_display, fake_surface, fake_surface, _))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, MakeCurrent_uses_correct_dummy_context)
{
    using namespace testing;

    EGLContext const dummy_ctx{reinterpret_cast<EGLContext>(0x17)};

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, EGL_NO_CONTEXT, _ ))
    .Times(Exactly(1))
    .WillOnce(Return(dummy_ctx));

    EXPECT_CALL(this->mock_egl, eglMakeCurrent(this->mock_egl.fake_egl_display, _, _, dummy_ctx))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}

TYPED_TEST(AndroidTestFramebufferInit, eglMakeCurrent_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglMakeCurrent(this->mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
    }, std::runtime_error);

}

TYPED_TEST(AndroidTestFramebufferInit, make_current_from_interface_calls_egl)
{
    using namespace testing;

    auto display = make_display_buffer<TypeParam>(this->native_win);

    EXPECT_CALL(this->mock_egl, eglMakeCurrent(this->mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_TRUE));

    display->make_current();
}

TYPED_TEST(AndroidTestFramebufferInit, make_current_failure_throws)
{
    using namespace testing;

    auto display = make_display_buffer<TypeParam>(this->native_win);

    EXPECT_CALL(this->mock_egl, eglMakeCurrent(this->mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        display->make_current();
    }, std::runtime_error);
}

namespace
{

template <class T>
class FakeResourceStore
{
public:
    void add_resource()
    {
        T const new_res{reinterpret_cast<T>(resources.size() + 1)};
        resources.insert(new_res);
        last_resource = new_res;
    }

    void remove_resource(T res)
    {
        if (resources.find(res) == resources.end())
            throw std::logic_error("FakeResourceStore: Trying to remove nonexistent element");

        resources.erase(res);
    }

    T* last_resource_ptr() { return &last_resource; }

    bool empty() { return resources.empty(); }

private:
    T last_resource;
    std::unordered_set<T> resources;
};

}

TYPED_TEST(AndroidTestFramebufferInit, eglContext_resources_freed)
{
    using namespace testing;

    typedef FakeResourceStore<EGLContext> FakeContextStore;
    FakeContextStore store;

    EXPECT_CALL(this->mock_egl, eglCreateContext(this->mock_egl.fake_egl_display, _, _, _ ))
    .Times(AtLeast(2))
    .WillRepeatedly(DoAll(InvokeWithoutArgs(&store, &FakeContextStore::add_resource),
                          ReturnPointee(store.last_resource_ptr())));

    EXPECT_CALL(this->mock_egl, eglDestroyContext(this->mock_egl.fake_egl_display, _))
    .Times(AtLeast(2))
    .WillRepeatedly(DoAll(WithArgs<1>(Invoke(&store, &FakeContextStore::remove_resource)),
                          Return(EGL_TRUE)));

    ASSERT_TRUE(store.empty());

    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
        ASSERT_FALSE(store.empty());
    }

    ASSERT_TRUE(store.empty());
}

TYPED_TEST(AndroidTestFramebufferInit, eglSurface_resources_freed)
{
    using namespace testing;

    typedef FakeResourceStore<EGLSurface> FakeSurfaceStore;
    FakeSurfaceStore store;

    EXPECT_CALL(this->mock_egl, eglCreateWindowSurface(this->mock_egl.fake_egl_display, _, _, _ ))
    .Times(AtLeast(1))
    .WillRepeatedly(DoAll(InvokeWithoutArgs(&store, &FakeSurfaceStore::add_resource),
                          ReturnPointee(store.last_resource_ptr())));

    EXPECT_CALL(this->mock_egl, eglCreatePbufferSurface(this->mock_egl.fake_egl_display, _, _ ))
    .Times(Exactly(1))
    .WillRepeatedly(DoAll(InvokeWithoutArgs(&store, &FakeSurfaceStore::add_resource),
                          ReturnPointee(store.last_resource_ptr())));

    EXPECT_CALL(this->mock_egl, eglDestroySurface(this->mock_egl.fake_egl_display, _))
    .Times(AtLeast(2))
    .WillRepeatedly(DoAll(WithArgs<1>(Invoke(&store, &FakeSurfaceStore::remove_resource)),
                          Return(EGL_TRUE)));

    ASSERT_TRUE(store.empty());

    {
        auto display = make_display_buffer<TypeParam>(this->native_win);
        ASSERT_FALSE(store.empty());
    }

    ASSERT_TRUE(store.empty());
}

TYPED_TEST(AndroidTestFramebufferInit, eglDisplay_is_terminated)
{
    using namespace testing;

    EXPECT_CALL(this->mock_egl, eglTerminate(this->mock_egl.fake_egl_display))
    .Times(Exactly(1));

    auto display = make_display_buffer<TypeParam>(this->native_win);
}
