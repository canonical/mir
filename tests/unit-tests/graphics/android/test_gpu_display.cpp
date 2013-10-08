/*
 * Copyright © 2013 Canonical Ltd.
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
#include "mir_test_doubles/mock_android_framebuffer_window.h"
#include "src/server/graphics/android/android_display.h"
#include "src/server/graphics/android/gpu_android_display_buffer_factory.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/stub_display_support_provider.h"
#include "mir_test_doubles/mock_egl.h"

#include <stdexcept>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class GPUFramebuffer : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        native_win = std::make_shared<NiceMock<mtd::MockAndroidFramebufferWindow>>();
        mock_egl.silence_uninteresting();
        mock_display_report = std::make_shared<mtd::MockDisplayReport>();
    }

    std::shared_ptr<mga::AndroidDisplay> create_display()
    {
        auto db_factory = std::make_shared<mga::GPUAndroidDisplayBufferFactory>();
        return std::make_shared<mga::AndroidDisplay>(native_win, db_factory, std::make_shared<mtd::StubDisplaySupportProvider>(), mock_display_report);
    }

    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;

    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    mtd::MockEGL mock_egl;
};

TEST_F(GPUFramebuffer, display_post_calls_swapbuffers_once)
{
    using namespace testing;
    auto display = create_display();

    EXPECT_CALL(mock_egl, eglSwapBuffers(mock_egl.fake_egl_display, mock_egl.fake_egl_surface))
        .Times(Exactly(1));

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

TEST_F(GPUFramebuffer, display_post_failure)
{
    using namespace testing;
    auto display = create_display();

    EXPECT_CALL(mock_egl, eglSwapBuffers(_,_))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW({
        display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
                buffer.post_update();
        });
    }, std::runtime_error);
}

TEST_F(GPUFramebuffer, framebuffer_correct_view_area)
{
    using namespace testing;
    auto display = create_display();
    unsigned int width = 456, height = 42111;

    EXPECT_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display,mock_egl.fake_egl_surface,EGL_WIDTH,_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>(width),
                        Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display,mock_egl.fake_egl_surface,EGL_HEIGHT,_))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>(height),
                        Return(EGL_TRUE)));

    std::vector<geom::Rectangle> areas;

    display->for_each_display_buffer([&areas](mg::DisplayBuffer& buffer)
    {
        areas.push_back(buffer.view_area());
    });

    ASSERT_EQ(1u, areas.size());

    auto area = areas[0];
    EXPECT_EQ(0u, area.top_left.x.as_uint32_t());
    EXPECT_EQ(0u, area.top_left.y.as_uint32_t());
    EXPECT_EQ(width, area.size.width.as_uint32_t());
    EXPECT_EQ(height, area.size.height.as_uint32_t());
}
