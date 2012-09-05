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

#include "mir/graphics/android/android_buffer_handle.h"
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

class AdaptorNativeWinProduction : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        /* set up common defaults */
        pf = mc::PixelFormat::rgba_8888;
        width = geom::Width(110);
        height = geom::Height(230);
        usage = mga::BufferUsage::use_hardware;
        stride = geom::Stride(300*4);

        anwb.height = (int) height.as_uint32_t();
        anwb.width = (int) width.as_uint32_t();
        anwb.stride = (int) stride.as_uint32_t();

    }

    ANativeWindowBuffer anwb;

    std::shared_ptr<mga::AndroidBufferHandle> buffer_handle;

    native_handle_t native_handle;
    geom::Width width;
    geom::Height height;
    geom::Stride stride;
    mga::BufferUsage usage;
    mc::PixelFormat pf;
};

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_height)
{
    using namespace testing;
    
    buffer_handle =  std::make_shared<mga::AndroidBufferHandle>(anwb, pf, usage );
    EXPECT_EQ(buffer_handle->height(), height);
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_width)
{
    using namespace testing;
    
    buffer_handle =  std::make_shared<mga::AndroidBufferHandle>(anwb, pf, usage );
    EXPECT_EQ(buffer_handle->width(), width);
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_stride)
{
    using namespace testing;
   
    buffer_handle =  std::make_shared<mga::AndroidBufferHandle>(anwb, pf, usage );

    EXPECT_EQ(buffer_handle->stride(), stride);
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_format)
{
    using namespace testing;
    
    buffer_handle =  std::make_shared<mga::AndroidBufferHandle>(anwb, pf, usage );
    EXPECT_EQ(buffer_handle->format(), pf);
}

TEST_F(AdaptorNativeWinProduction, native_win_has_correct_usage)
{
    using namespace testing;
    
    buffer_handle =  std::make_shared<mga::AndroidBufferHandle>(anwb, pf, usage );
    EXPECT_EQ(buffer_handle->usage(), usage);
}

