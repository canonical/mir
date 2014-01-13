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

#include "src/platform/graphics/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_buffer.h"
#include "hwc_struct_helper-inl.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace mir
{
namespace test
{
namespace doubles
{
struct MockRenderable : public mg::Renderable
{
    MOCK_CONST_METHOD0(buffer, std::shared_ptr<mg::Buffer>());
    MOCK_CONST_METHOD0(alpha_enabled, bool());
    MOCK_CONST_METHOD0(screen_position, geom::Rectangle());
};
}
}
}

class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
        native_handle_1->anwb()->width = width;
        native_handle_1->anwb()->height = height;
        native_handle_2 = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>();


    }

    geom::Size display_size{111, 222};
    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    bool alpha_enabled{true};
    int width{432};
    int height{876};
    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> native_handle_2;
    testing::NiceMock<mtd::MockRenderable> mock_renderable;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
};

TEST_F(HWCLayerListTest, empty_hwc_layer_list)
{
    using namespace testing;
    mga::LayerList layerlist;

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(1, list->numHwLayers);
    ASSERT_EQ(HWC_FRAMEBUFFER, list->hwLayers[0].compositionType);
    ASSERT_EQ(HWC_SKIP_LAYER, list->hwLayers[0].flags);
}

TEST_F(HWCLayerListTest, one_composition_layer)
{
    using namespace testing;
    mga::LayerList layerlist;
    auto list = layerlist.native_list();
  
    mga::CompositionLayer surface_layer{mock_renderable, display_size}
    std::list<std::shared_ptr<mg::Renderable>> list(
    {
        surface_layer
    });
    layerlist.update_overlays(list);

    //dont realloc
    EXPECT_EQ(list, layerlist.native_list());

    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(1, list->numHwLayers);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
}

TEST_F(HWCLayerListTest, fb_target_set)
{
    mga::LayerList layerlist;
    layerlist.set_fb_target(native_handle_1);

    auto list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);

    mga::ForceGLLayer skip_layer;
    mga::FramebufferLayer target_layer(*native_handle_1);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fb_target_with_one_layer)
{
    mga::LayerList layerlist;

    mga::CompositionLayer surface_layer{mock_renderable, display_size}
    std::list<std::shared_ptr<mg::Renderable>> list(
    {
        surface_layer
    });

    layerlist.update_overlays(list)
    layerlist.set_fb_target(native_handle_1);

    auto list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);
    mga::FramebufferLayer target_layer(*native_handle_1);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, overlay_layer_changes)
{
    mga::LayerList layerlist;

    mga::CompositionLayer surface_layer{mock_renderable, display_size}
    std::list<std::shared_ptr<mg::Renderable>> list(
    {
        surface_layer
    });

    layerlist.update_overlays(list)
    layerlist.set_fb_target(native_handle_1);

    auto list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);
    mga::FramebufferLayer target_layer(*native_handle_1);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, hwc_list_throws_if_no_fb_layer)
{
    using namespace testing;
    mga::LayerList layerlist({});
    EXPECT_THROW({
        layerlist.set_fb_target(native_handle_2);
    }, std::runtime_error);

    EXPECT_THROW({
        layerlist.framebuffer_fence();
    }, std::runtime_error);
}

TEST_F(HWCLayerListTest, get_fb_fence)
{
    int release_fence = 381;
    mga::LayerList layerlist({
        mga::CompositionLayer(mock_renderable, display_size), 
        mga::FramebufferLayer(*native_handle_1)});

    auto list = layerlist.native_list();
    list->hwLayers[1].releaseFenceFd = release_fence;

    EXPECT_EQ(release_fence, layerlist.framebuffer_fence());
}

TEST_F(HWCLayerListTest, hwc_fb_target_update)
{
    using namespace testing;

    int handle_fence = 442;
    EXPECT_CALL(*native_handle_2, copy_fence())
        .Times(1)
        .WillOnce(Return(handle_fence));

    mga::LayerList layerlist({
        mga::CompositionLayer(mock_renderable, display_size), 
        mga::FramebufferLayer(*native_handle_1)});
    layerlist.set_fb_target(native_handle_2);

    auto list = layerlist.native_list();
    ASSERT_EQ(2u, list->numHwLayers);
    EXPECT_EQ(native_handle_1->handle(), list->hwLayers[0].handle);
    EXPECT_EQ(-1, list->hwLayers[0].acquireFenceFd);
    EXPECT_EQ(list->hwLayers[1].handle, native_handle_2->handle());
    EXPECT_EQ(handle_fence, list->hwLayers[1].acquireFenceFd);
}
