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
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_display_device.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir_test_doubles/stub_driver_interpreter.h"

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

class AndroidDisplayTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        mock_egl.silence_uninteresting();

        visual_id = 5;
        mock_display_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
        stub_display_device = std::make_shared<mtd::StubDisplayDevice>(display_size);
        auto stub_driver_interpreter = std::make_shared<mtd::StubDriverInterpreter>(display_size, visual_id);
        native_win = std::make_shared<mg::android::MirNativeWindow>(stub_driver_interpreter);
        db_factory = std::make_shared<mga::DisplayBufferFactory>();
    }

    int visual_id;
    std::shared_ptr<mga::DisplayBufferFactory> db_factory;
    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<ANativeWindow> native_win;
    std::shared_ptr<mtd::StubDisplayDevice> stub_display_device;
    mtd::MockEGL mock_egl;
};

TEST_F(AndroidDisplayTest, eglChooseConfig_attributes)
{
    using namespace testing;

    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
            SaveArg<1>(&attr),
            SetArgPointee<2>(mock_egl.fake_configs),
            SetArgPointee<4>(mock_egl.fake_configs_num),
            Return(EGL_TRUE)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report);

    int i=0;
    bool surface_bit_correct = false;
    bool renderable_bit_correct = false;
    while(attr[i] != EGL_NONE)
    {
        if ((attr[i] == EGL_SURFACE_TYPE) && (attr[i+1] == EGL_WINDOW_BIT))
        {
            surface_bit_correct = true;
        }
        if ((attr[i] == EGL_RENDERABLE_TYPE) && (attr[i+1] == EGL_OPENGL_ES2_BIT))
        {
            renderable_bit_correct = true;
        }
        i++;
    };

    EXPECT_EQ(EGL_NONE, attr[i]);
    EXPECT_TRUE(surface_bit_correct);
    EXPECT_TRUE(renderable_bit_correct);
}

TEST_F(AndroidDisplayTest, queries_with_enough_room_for_all_potential_cfg)
{
    using namespace testing;

    int num_cfg = 43;
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglGetConfigs(mock_egl.fake_egl_display, NULL, 0, _))
    .Times(AtLeast(1))
    .WillOnce(DoAll(
                  SetArgPointee<3>(num_cfg),
                  Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, num_cfg, _))
    .Times(AtLeast(1))
    .WillOnce(DoAll(
                  SaveArg<1>(&attr),
                  SetArgPointee<2>(mock_egl.fake_configs),
                  SetArgPointee<4>(mock_egl.fake_configs_num),
                  Return(EGL_TRUE)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report);

    /* should be able to ref this spot */
    EGLint test_last_spot = attr[num_cfg-1];
    EXPECT_EQ(test_last_spot, test_last_spot);

}

TEST_F(AndroidDisplayTest, creates_with_proper_visual_id_mixed_valid_invalid)
{
    using namespace testing;

    EGLConfig cfg, chosen_cfg;

    int bad_id = visual_id + 1;

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(
            SetArgPointee<3>(bad_id),
            Return(EGL_TRUE)))
        .WillOnce(DoAll(
            SetArgPointee<3>(visual_id),
            SaveArg<1>(&cfg),
            Return(EGL_TRUE)))
        .WillRepeatedly(DoAll(
            SetArgPointee<3>(bad_id),
            Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglCreateContext(_,_,_,_))
        .WillRepeatedly(DoAll(
            SaveArg<1>(&chosen_cfg),
            Return(mock_egl.fake_egl_context)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report);

    Mock::VerifyAndClearExpectations(&mock_egl);
    EXPECT_EQ(cfg, chosen_cfg);
}

TEST_F(AndroidDisplayTest, without_proper_visual_id_throws)
{
    using namespace testing;
    int bad_id = visual_id + 1;
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
        .WillRepeatedly(DoAll(
            SetArgPointee<3>(bad_id),
            Return(EGL_TRUE)));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report);
    }, std::runtime_error );
}

TEST_F(AndroidDisplayTest, eglGetDisplay)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, eglGetDisplay_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .Times(1)
        .WillOnce(Return((EGLDisplay)EGL_NO_DISPLAY));

    EXPECT_THROW({
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidDisplayTest, eglInitialize)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, eglInitialize_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidDisplayTest, eglInitialize_failure_bad_major_version)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(DoAll(
              SetArgPointee<1>(2),
              Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidDisplayTest, eglInitialize_failure_bad_minor_version)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(DoAll(
              SetArgPointee<2>(2),
              Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidDisplayTest, eglCreateWindowSurface_nullarg)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, NULL))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, eglCreateWindowSurface_uses_native_window_type)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, native_win.get(),_))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, eglCreateWindowSurface_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display,_,_,_))
        .Times(1)
        .WillOnce(Return((EGLSurface)NULL));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error);
}

/* create context stuff */
TEST_F(AndroidDisplayTest, CreateContext_window_cfg_matches_context_cfg)
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

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );

    EXPECT_EQ(chosen_cfg, cfg);
}

TEST_F(AndroidDisplayTest, CreateContext_contexts_are_shared)
{
    using namespace testing;

    EGLContext const shared_ctx{reinterpret_cast<EGLContext>(0x17)};

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, EGL_NO_CONTEXT,_))
        .Times(1)
        .WillOnce(Return(shared_ctx));

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, shared_ctx,_))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

namespace
{

ACTION_P(AppendContextAttrPtr, vec)
{
    vec->push_back(arg3);
}

}

TEST_F(AndroidDisplayTest, CreateContext_context_attr_null_terminated)
{
    using namespace testing;

    std::vector<EGLint const*> context_attr_ptrs;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(AtLeast(2))
        .WillRepeatedly(DoAll(
            AppendContextAttrPtr(&context_attr_ptrs),
            Return((EGLContext)mock_egl.fake_egl_context)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );

    for (auto context_attr : context_attr_ptrs)
    {
        int i = 0;
        ASSERT_NE(nullptr, context_attr);
        while(context_attr[i++] != EGL_NONE);
    }
}

TEST_F(AndroidDisplayTest, CreateContext_context_uses_client_version_2)
{
    using namespace testing;

    std::vector<EGLint const*> context_attr_ptrs;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(AtLeast(2))
        .WillRepeatedly(
            DoAll(AppendContextAttrPtr(&context_attr_ptrs),
            Return((EGLContext)mock_egl.fake_egl_context)));

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );

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

TEST_F(AndroidDisplayTest, CreateContext_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
        .Times(1)
        .WillOnce(Return((EGLContext)EGL_NO_CONTEXT));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error   );
}

TEST_F(AndroidDisplayTest, MakeCurrent_uses_correct_pbuffer_surface)
{
    using namespace testing;
    EGLSurface fake_surface = (EGLSurface) 0x715;

    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(mock_egl.fake_egl_display, _, _))
        .Times(1)
        .WillOnce(Return(fake_surface));
    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, fake_surface, fake_surface, _))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, MakeCurrent_uses_correct_dummy_context)
{
    using namespace testing;

    EGLContext const dummy_ctx{reinterpret_cast<EGLContext>(0x17)};

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, EGL_NO_CONTEXT, _ ))
        .Times(1)
        .WillOnce(Return(dummy_ctx));

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, dummy_ctx))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, eglMakeCurrent_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, _))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error);

}

TEST_F(AndroidDisplayTest, make_current_from_interface_calls_egl)
{
    using namespace testing;

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, _))
        .Times(1)
        .WillOnce(Return(EGL_TRUE));

    display.for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.make_current();
    });

    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(AndroidDisplayTest, make_current_failure_throws)
{
    using namespace testing;

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );

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

TEST_F(AndroidDisplayTest, eglContext_resources_freed)
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
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
        ASSERT_FALSE(store.empty());
    }

    ASSERT_TRUE(store.empty());
}

TEST_F(AndroidDisplayTest, eglSurface_resources_freed)
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
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
        ASSERT_FALSE(store.empty());
    }

    ASSERT_TRUE(store.empty());
}

TEST_F(AndroidDisplayTest, display_termination) 
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        .Times(1);
    EXPECT_CALL(mock_egl, eglTerminate(mock_egl.fake_egl_display))
        .Times(1);

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}


TEST_F(AndroidDisplayTest, startup_logging_ok)
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

    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
}

TEST_F(AndroidDisplayTest, startup_logging_error_because_of_surface_creation_failure)
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
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error);
}

TEST_F(AndroidDisplayTest, startup_logging_error_because_of_makecurrent)
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
        mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
    }, std::runtime_error);
}

//we only have single display and single mode on android for the time being
TEST_F(AndroidDisplayTest, android_display_configuration_info)
{
    mga::AndroidDisplay display(native_win, db_factory, stub_display_device, mock_display_report );
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
