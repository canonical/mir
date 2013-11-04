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

#include "src/server/graphics/android/framebuffers.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_alloc_adaptor.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"

#include <future>
#include <initializer_list>
#include <thread>
#include <stdexcept>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

namespace
{
class PostingFBBundleTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;
        buffer1 = std::make_shared<mtd::MockBuffer>();
        buffer2 = std::make_shared<mtd::MockBuffer>();
        buffer3 = std::make_shared<mtd::MockBuffer>();
        mock_allocator = std::make_shared<mtd::MockAllocAdaptor>();
        mock_hwc_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        EXPECT_CALL(*mock_allocator, alloc_buffer(_,_,_))
            .Times(AtLeast(0))
            .WillOnce(Return(buffer1))
            .WillOnce(Return(buffer2))
            .WillRepeatedly(Return(buffer3));
    }

    std::shared_ptr<mtd::MockAllocAdaptor> mock_allocator;
    std::shared_ptr<mg::Buffer> buffer1;
    std::shared_ptr<mg::Buffer> buffer2;
    std::shared_ptr<mg::Buffer> buffer3;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_hwc_device;
};

static int const display_width = 180;
static int const display_height = 1010101;

static int display_attribute_handler(struct hwc_composer_device_1*, int, uint32_t,
                                     const uint32_t* attribute_list, int32_t* values)
{
    EXPECT_EQ(attribute_list[0], HWC_DISPLAY_WIDTH);
    EXPECT_EQ(attribute_list[1], HWC_DISPLAY_HEIGHT);
    EXPECT_EQ(attribute_list[2], HWC_DISPLAY_NO_ATTRIBUTE);

    values[0] = display_width;
    values[1] = display_height;
    return 0;
}
}

TEST_F(PostingFBBundleTest, hwc_fb_size_allocation)
{
    using namespace testing;

    unsigned int hwc_configs = 0xA1;
    EXPECT_CALL(*mock_hwc_device, getDisplayConfigs_interface(mock_hwc_device.get(),HWC_DISPLAY_PRIMARY,_,Pointee(1)))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(hwc_configs), Return(0)));
    EXPECT_CALL(*mock_hwc_device, getDisplayAttributes_interface(mock_hwc_device.get(), HWC_DISPLAY_PRIMARY, hwc_configs,_,_))
        .Times(1)
        .WillOnce(Invoke(display_attribute_handler));
   
    auto display_size = mir::geometry::Size{display_width, display_height};
    EXPECT_CALL(*mock_allocator, alloc_buffer(display_size, _, mga::BufferUsage::use_framebuffer_gles))
        .Times(2);

    mga::Framebuffers framebuffers(mock_allocator, mock_hwc_device);
}

#if 0 
TEST_F(HWC11Device, hwc_version_11_format_selection)
{
    using namespace testing;
    EGLint const expected_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_FRAMEBUFFER_TARGET_ANDROID, EGL_TRUE,
        EGL_NONE
    };

    int visual_id = HAL_PIXEL_FORMAT_BGRA_8888;
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);
    EGLConfig fake_egl_config = reinterpret_cast<EGLConfig>(0x44);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(fake_display,mtd::AttrMatches(expected_egl_config_attr),_,1,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<2>(fake_egl_config), SetArgPointee<4>(1), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(fake_display, fake_egl_config, EGL_NATIVE_VISUAL_ID, _))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<3>(visual_id), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);
 
    mga::HWC11Device device(mock_device, mock_hwc_layers, mock_display_device, mock_vsync);
    EXPECT_EQ(geom::PixelFormat::argb_8888, device.display_format());
}
#endif


#if 0
TEST_F(PostingFBBundleTest, simple_swaps_returns_valid)
{
    mga::PostingFBBundle fb_swapper(mock_allocator, mock_fb_dev);

    auto test_buffer = fb_swapper.compositor_acquire();

    EXPECT_TRUE((test_buffer == buffer1) || (test_buffer == buffer2));
} 

TEST_F(PostingFBBundleTest, simple_swaps_return_aba_pattern)
{
    mga::PostingFBBundle fb_swapper(mock_allocator, mock_fb_dev);

    auto test_buffer_1 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_1);

    auto test_buffer_2 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_2);

    auto test_buffer_3 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_3);

    EXPECT_EQ(test_buffer_1, test_buffer_3);
    EXPECT_NE(test_buffer_1, test_buffer_2);
} 

TEST_F(PostingFBBundleTest, triple_swaps_return_abcab_pattern)
{
    mga::PostingFBBundle fb_swapper(mock_allocator, mock_fb_dev);

    auto test_buffer_1 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_1);

    auto test_buffer_2 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_2);

    auto test_buffer_3 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_3);

    auto test_buffer_4 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_4);

    auto test_buffer_5 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_5);

    EXPECT_EQ(test_buffer_1, test_buffer_4);
    EXPECT_NE(test_buffer_1, test_buffer_2);
    EXPECT_NE(test_buffer_1, test_buffer_3);

    EXPECT_EQ(test_buffer_2, test_buffer_5);
    EXPECT_NE(test_buffer_2, test_buffer_1);
    EXPECT_NE(test_buffer_2, test_buffer_3);
}

TEST_F(PostingFBBundleTest, synctest)
{
    mga::PostingFBBundle fb_swapper(mock_allocator, mock_fb_dev);

    std::vector<std::shared_ptr<mg::Buffer>> blist;
    std::mutex mut;
    for(auto i=0u; i < 150; ++i)
    {
        auto buf1 = fb_swapper.compositor_acquire();
        auto buf2 = fb_swapper.compositor_acquire();

        auto f = std::async(std::launch::async,
                            [&]()
                            {
                                /* driver will release in order */
                                fb_swapper.compositor_release(buf1);
                                fb_swapper.compositor_release(buf2);
                            });

        //this line will wait if f has not run
        auto buf3 = fb_swapper.compositor_acquire();
        f.wait(); //make sure buf3 is released after buf2 to maintain order
        fb_swapper.compositor_release(buf3);

        blist.push_back(buf1);
        blist.push_back(buf2);
        blist.push_back(buf3);
    }

    //This chunk of code makes sure "ABABA..." or "BABAB..." pattern was produced 
    auto modcounter = 0u;
    for(auto i : test_buffers)
    {
        if (blist[0] == i)
        {
            break;
        }
        modcounter++;
    }

    for(auto i : blist)
    {
        EXPECT_EQ(test_buffers[(modcounter % test_buffers.size())], i);
        modcounter++;
    }
}
#endif
