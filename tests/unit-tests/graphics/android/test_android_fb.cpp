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

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display_configuration.h"
#include "mir/logging/logger.h"
#include "src/server/graphics/android/android_display.h"
#include "src/server/graphics/android/display_buffer_factory.h"
#include "mir_test_doubles/mock_android_framebuffer_window.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_display_support_provider.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace ml=mir::logging;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

struct DummyGPUDisplayType {};
struct DummyHWCDisplayType {};

static geom::Size const display_size{433,232};

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

        mock_display_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
        stub_display_support = std::make_shared<mtd::StubDisplaySupportProvider>(display_size);
        db_factory = std::make_shared<mga::DisplayBufferFactory>();
    }

    std::shared_ptr<mga::DisplayBufferFactory> db_factory;
    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    std::shared_ptr<mtd::StubDisplaySupportProvider> stub_display_support;
    mtd::MockEGL mock_egl;
};

TEST_F(AndroidTestFramebufferInit, eglGetDisplay)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, eglGetDisplay_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .Times(1)
        .WillOnce(Return((EGLDisplay)EGL_NO_DISPLAY));

    EXPECT_THROW({
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize_failure_bad_major_version)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(DoAll(
              SetArgPointee<1>(2),
              Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize_failure_bad_minor_version)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(DoAll(
              SetArgPointee<2>(2),
              Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_requests_config)
{
    using namespace testing;
    EGLConfig fake_config = (EGLConfig) 0x3432;
    EXPECT_CALL(*native_win, android_display_egl_config(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(fake_config));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, fake_config, _, _))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_nullarg)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, NULL))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_uses_native_window_type)
{
    using namespace testing;
    EGLNativeWindowType egl_window = (EGLNativeWindowType)0x4443;

    EXPECT_CALL(*native_win, android_native_window_type())
        .Times(1)
        .WillOnce(Return(egl_window));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, egl_window,_))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display,_,_,_))
        .Times(1)
        .WillOnce(Return((EGLSurface)NULL));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error);
}

/* create context stuff */
TEST_F(AndroidTestFramebufferInit, CreateContext_window_cfg_matches_context_cfg)
{
    using namespace testing;

    EGLConfig chosen_cfg, cfg;
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, _))
        .Times(1)
        .WillOnce(DoAll(
              SaveArg<1>(&chosen_cfg),
              Return((EGLSurface)mock_egl.fake_egl_surface)));

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, Ne(EGL_NO_CONTEXT),_))
        .Times(1)
        .WillOnce(DoAll(
              SaveArg<1>(&cfg),
              Return((EGLContext)mock_egl.fake_egl_context)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );

    EXPECT_EQ(chosen_cfg, cfg);
}

TEST_F(AndroidTestFramebufferInit, CreateContext_contexts_are_shared)
{
    using namespace testing;

    EGLContext const shared_ctx{reinterpret_cast<EGLContext>(0x17)};

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, EGL_NO_CONTEXT,_))
        .Times(1)
        .WillOnce(Return(shared_ctx));

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, shared_ctx,_))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

namespace
{

ACTION_P(AppendContextAttrPtr, vec)
{
    vec->push_back(arg3);
}

}

TEST_F(AndroidTestFramebufferInit, CreateContext_context_attr_null_terminated)
{
    using namespace testing;

    std::vector<EGLint const*> context_attr_ptrs;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(AtLeast(2))
        .WillRepeatedly(DoAll(
            AppendContextAttrPtr(&context_attr_ptrs),
            Return((EGLContext)mock_egl.fake_egl_context)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );

    for (auto context_attr : context_attr_ptrs)
    {
        int i = 0;
        ASSERT_NE(nullptr, context_attr);
        while(context_attr[i++] != EGL_NONE);
    }
}

TEST_F(AndroidTestFramebufferInit, CreateContext_context_uses_client_version_2)
{
    using namespace testing;

    std::vector<EGLint const*> context_attr_ptrs;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(AtLeast(2))
        .WillRepeatedly(
            DoAll(AppendContextAttrPtr(&context_attr_ptrs),
            Return((EGLContext)mock_egl.fake_egl_context)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );

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

TEST_F(AndroidTestFramebufferInit, CreateContext_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(1)
        .WillOnce(Return((EGLContext)EGL_NO_CONTEXT));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, MakeCurrent_uses_correct_pbuffer_surface)
{
    using namespace testing;
    EGLSurface fake_surface = (EGLSurface) 0x715;

    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(Return(fake_surface));
    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, fake_surface, fake_surface, _))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, MakeCurrent_uses_correct_dummy_context)
{
    using namespace testing;

    EGLContext const dummy_ctx{reinterpret_cast<EGLContext>(0x17)};

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, EGL_NO_CONTEXT, _ ))
        .Times(1)
        .WillOnce(Return(dummy_ctx));

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, dummy_ctx))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, eglMakeCurrent_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, _))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error);

}

TEST_F(AndroidTestFramebufferInit, make_current_from_interface_calls_egl)
{
    using namespace testing;

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, _))
        .Times(1)
        .WillOnce(Return(EGL_TRUE));

    display.for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.make_current();
    });

    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(AndroidTestFramebufferInit, make_current_failure_throws)
{
    using namespace testing;

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, _))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        display.for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            buffer.make_current();
        });
    }, std::runtime_error);

    Mock::VerifyAndClearExpectations(&mock_egl);
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

TEST_F(AndroidTestFramebufferInit, eglContext_resources_freed)
{
    using namespace testing;

    typedef FakeResourceStore<EGLContext> FakeContextStore;
    FakeContextStore store;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(AtLeast(2))
        .WillRepeatedly(DoAll(
            InvokeWithoutArgs(&store, &FakeContextStore::add_resource),
            ReturnPointee(store.last_resource_ptr())));

    EXPECT_CALL(mock_egl, eglDestroyContext(mock_egl.fake_egl_display, _))
        .Times(AtLeast(2))
        .WillRepeatedly(DoAll(
            WithArgs<1>(Invoke(&store, &FakeContextStore::remove_resource)),
            Return(EGL_TRUE)));

    ASSERT_TRUE(store.empty());

    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
        ASSERT_FALSE(store.empty());
    }

    ASSERT_TRUE(store.empty());
}

TEST_F(AndroidTestFramebufferInit, eglSurface_resources_freed)
{
    using namespace testing;

    typedef FakeResourceStore<EGLSurface> FakeSurfaceStore;
    FakeSurfaceStore store;

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, _ ))
    .Times(AtLeast(1))
    .WillRepeatedly(DoAll(
        InvokeWithoutArgs(&store, &FakeSurfaceStore::add_resource),
        ReturnPointee(store.last_resource_ptr())));

    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(mock_egl.fake_egl_display, _, _ ))
        .Times(1)
        .WillRepeatedly(DoAll(
            InvokeWithoutArgs(&store, &FakeSurfaceStore::add_resource),
            ReturnPointee(store.last_resource_ptr())));

    EXPECT_CALL(mock_egl, eglDestroySurface(mock_egl.fake_egl_display, _))
        .Times(AtLeast(2))
        .WillRepeatedly(DoAll(
            WithArgs<1>(Invoke(&store, &FakeSurfaceStore::remove_resource)),
            Return(EGL_TRUE)));

    ASSERT_TRUE(store.empty());

    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
        ASSERT_FALSE(store.empty());
    }

    ASSERT_TRUE(store.empty());
}

TEST_F(AndroidTestFramebufferInit, display_termination) 
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        .Times(1);
    EXPECT_CALL(mock_egl, eglTerminate(mock_egl.fake_egl_display))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}


TEST_F(AndroidTestFramebufferInit, startup_logging_ok)
{
    using namespace testing;
    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_egl_configuration(_,_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(1);

    EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,_))
        .Times(AtLeast(1));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
}

TEST_F(AndroidTestFramebufferInit, startup_logging_error_because_of_surface_creation_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(0);

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(_,_,_,_))
        .Times(1)
        .WillOnce(Return(EGL_NO_SURFACE));

    EXPECT_THROW({
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error);
}

TEST_F(AndroidTestFramebufferInit, startup_logging_error_because_of_makecurrent)
{
    using namespace testing;

    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(0);

    EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,_))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW({
        mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    }, std::runtime_error);
}

//we only have single display and single mode on android for the time being
TEST_F(AndroidTestFramebufferInit, android_display_configuration_info)
{
    mga::AndroidDisplay display(native_win, db_factory, stub_display_support, mock_display_report );
    auto config = display.configuration();

    std::vector<mg::DisplayConfigurationOutput> configurations;
    config->for_each_output([&](mg::DisplayConfigurationOutput const& config)
    {
        configurations.push_back(config);
    });

    ASSERT_EQ(1u, configurations.size());
    auto& disp_conf = configurations[0];
    ASSERT_EQ(1u, disp_conf.modes.size());
    auto& disp_mode = disp_conf.modes[0];
    EXPECT_EQ(display_size, disp_mode.size);
    //TODO fill refresh rate accordingly

    EXPECT_EQ(mg::DisplayConfigurationOutputId{1}, disp_conf.id); 
    EXPECT_EQ(mg::DisplayConfigurationCardId{0}, disp_conf.card_id); 
    EXPECT_TRUE(disp_conf.connected);
    EXPECT_TRUE(disp_conf.used);
    auto origin = geom::Point{0,0}; 
    EXPECT_EQ(origin, disp_conf.top_left);
    EXPECT_EQ(0, disp_conf.current_mode_index);
    //TODO fill physical_size_mm fields accordingly;
}
