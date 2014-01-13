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
#include "src/platform/graphics/android/hwc_layers.h"
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

TEST_F(HWCLayerListTest, hwc10_list_defaults)
{
    using namespace testing;
    mga::HWC10LayerList layerlist;

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(1, list->numHwLayers);

    mga::ForceGLLayer surface_layer;
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
}

TEST_F(HWCLayerListTest, hwc_one_composition_layer)
{
    using namespace testing;
    mga::HWC10LayerList layerlist;
    auto list = layerlist.native_list();
  
    std::list<std::shared_ptr<mg::Renderable>> renderable_list(
    {
        mt::fake_shared(mock_renderable)
    });
    layerlist.update_composition_layers(renderable_list);

    //dont realloc
    EXPECT_EQ(list, layerlist.native_list());

    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(1, list->numHwLayers);
    mga::CompositionLayer surface_layer{mock_renderable, display_size};
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
}

TEST_F(HWCLayerListTest, fbtarget_list_defaults)
{
    using namespace testing;
    mga::FBTargetLayerList layerlist;

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(2, list->numHwLayers);

    mga::ForceGLLayer surface_layer;
    mga::FramebufferLayer target_layer;
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fbtarget_update_one_composition_layer)
{
    using namespace testing;
    mga::FBTargetLayerList layerlist;
    auto list = layerlist.native_list();
  
    std::list<std::shared_ptr<mg::Renderable>> renderable_list(
    {
        mt::fake_shared(mock_renderable)
    });
    layerlist.update_composition_layers(renderable_list);

    //dont realloc
    EXPECT_EQ(list, layerlist.native_list());

    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(2, list->numHwLayers);

    mga::CompositionLayer surface_layer{mock_renderable, display_size};
    mga::FramebufferLayer target_layer;
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fb_target_set)
{
    mga::FBTargetLayerList layerlist;
    auto list = layerlist.native_list();

    layerlist.set_fb_target(native_handle_1);

    //dont realloc
    EXPECT_EQ(list, layerlist.native_list());

    ASSERT_EQ(2, list->numHwLayers);
    mga::ForceGLLayer skip_layer;
    mga::FramebufferLayer target_layer(*native_handle_1);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fbtarget_update_two_composition_layers_and_fb_target)
{
    using namespace testing;
    mga::FBTargetLayerList layerlist;
    auto list = layerlist.native_list();
  
    std::list<std::shared_ptr<mg::Renderable>> renderable_list(
    {
        mt::fake_shared(mock_renderable),
        mt::fake_shared(mock_renderable)
    });
    layerlist.update_composition_layers(renderable_list);
    layerlist.set_fb_target(native_handle_1);

    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(3, list->numHwLayers);

    mga::CompositionLayer surface_layer{mock_renderable, display_size};
    mga::FramebufferLayer target_layer;
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[2]));
}

TEST_F(HWCLayerListTest, get_fb_fence)
{
    int release_fence = 381;
    mga::FBTargetLayerList layerlist;

    auto list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);
    list->hwLayers[1].releaseFenceFd = release_fence;

    EXPECT_EQ(release_fence, layerlist.framebuffer_fence());
}
