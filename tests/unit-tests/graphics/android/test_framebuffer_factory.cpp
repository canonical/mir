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

#include "mir_test_doubles/mock_alloc_adaptor.h"
#include "src/server/graphics/android/default_framebuffer_factory.h"
#include "src/server/graphics/android/android_buffer_handle.h"
#include "src/server/graphics/android/native_buffer_handle.h"
#include "src/server/graphics/android/display_info_provider.h"

#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{
class MockDisplayInfoProvider : public mga::DisplayInfoProvider
{
public:
    MOCK_METHOD0(display_size, geom::Size());
    MOCK_METHOD0(display_format, geom::PixelFormat());
    MOCK_METHOD0(number_of_framebuffers_available, unsigned int());
};
}

class FBFactory  : public ::testing::Test
{
public:
    FBFactory()
        : mock_buffer_allocator(std::make_shared<mtd::MockAllocAdaptor>()),
          mock_display_info_provider(std::make_shared<MockDisplayInfoProvider>()),
          fake_fb_num(2)
    {
        using namespace testing;
        ON_CALL(*mock_display_info_provider, display_format())
            .WillByDefault(Return(geom::PixelFormat::abgr_8888));
        ON_CALL(*mock_display_info_provider, display_size())
            .WillByDefault(Return(geom::Size{geom::Width{2}, geom::Height{3}}));
        ON_CALL(*mock_display_info_provider, number_of_framebuffers_available())
            .WillByDefault(Return(fake_fb_num));
        ON_CALL(*mock_buffer_allocator, alloc_buffer(_,_,_))
            .WillByDefault(Return(std::shared_ptr<mga::AndroidBufferHandle>()));
    }

    std::shared_ptr<mtd::MockAllocAdaptor> mock_buffer_allocator;
    std::shared_ptr<MockDisplayInfoProvider> mock_display_info_provider;
    unsigned int const fake_fb_num;
};

TEST_F(FBFactory, test_native_window_creation_uses_size)
{
    using namespace testing;

    mga::DefaultFramebufferFactory factory(mock_buffer_allocator);

    geom::Width disp_width{44};
    geom::Height disp_height{4567654};
    geom::Size disp_size{disp_width, disp_height};   
 
    EXPECT_CALL(*mock_display_info_provider, display_size())
        .Times(1)
        .WillOnce(Return(disp_size));
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(disp_size,_,_))
        .Times(fake_fb_num);
 
    factory.create_fb_native_window(mock_display_info_provider);
} 

TEST_F(FBFactory, test_native_window_creation_specifies_buffer_type)
{
    using namespace testing;

    mga::DefaultFramebufferFactory factory(mock_buffer_allocator);

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(_,_,mga::BufferUsage::use_framebuffer_gles))
        .Times(fake_fb_num);
 
    factory.create_fb_native_window(mock_display_info_provider);
} 

//note: @kdub imo, the hwc api has a hole in it that it doesn't allow query for format. surfaceflinger code
//            makes note of this api hole in its comments too. It always uses rgba8888, which we will do too.
TEST_F(FBFactory, test_native_window_creation_uses_rgba8888)
{
    using namespace testing;

    mga::DefaultFramebufferFactory factory(mock_buffer_allocator);
    geom::PixelFormat pf = geom::PixelFormat::abgr_8888; 
 
    EXPECT_CALL(*mock_display_info_provider, display_format())
        .Times(1)
        .WillOnce(Return(pf));
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(_,pf,_))
        .Times(fake_fb_num);
 
    factory.create_fb_native_window(mock_display_info_provider);
}
