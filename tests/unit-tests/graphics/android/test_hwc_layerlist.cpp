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

#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/stub_buffer.h"
#include "src/platforms/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "hwc_struct_helpers.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{
struct LayerListTest : public testing::Test
{
    LayerListTest() :
        layer_adapter{std::make_shared<mga::IntegerSourceCrop>()},
        buffer1{std::make_shared<mtd::StubBuffer>()},
        buffer2{std::make_shared<mtd::StubBuffer>()},
        renderables{std::make_shared<mtd::StubRenderable>(buffer1),
                    std::make_shared<mtd::StubRenderable>(buffer2),
                    std::make_shared<mtd::StubRenderable>()}
    {}

    std::shared_ptr<mga::LayerAdapter> layer_adapter;
    std::shared_ptr<mtd::StubBuffer> buffer1;
    std::shared_ptr<mtd::StubBuffer> buffer2;
    std::list<std::shared_ptr<mg::Renderable>> renderables;
};
}

TEST_F(LayerListTest, list_defaults)
{
    mga::LayerList layerlist{layer_adapter, {}};

    auto list = layerlist.native_list().lock();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    EXPECT_EQ(layerlist.begin(), layerlist.additional_layers_begin());
    EXPECT_EQ(std::distance(layerlist.additional_layers_begin(), layerlist.end()), 2);
}

TEST_F(LayerListTest, list_iterators)
{
    size_t additional_layers = 2;
    mga::LayerList list(layer_adapter, {});
    EXPECT_EQ(std::distance(list.begin(), list.end()), additional_layers);
    EXPECT_EQ(std::distance(list.additional_layers_begin(), list.end()), additional_layers);
    EXPECT_EQ(std::distance(list.begin(), list.additional_layers_begin()), 0);

    additional_layers = 1;
    mga::LayerList list2(layer_adapter, renderables);
    EXPECT_EQ(std::distance(list2.begin(), list2.end()), additional_layers + renderables.size());
    EXPECT_EQ(std::distance(list2.additional_layers_begin(), list2.end()), additional_layers);
    EXPECT_EQ(std::distance(list2.begin(), list2.additional_layers_begin()), renderables.size());

    additional_layers = 0;
    mga::LayerList list3(std::make_shared<mga::Hwc10Adapter>(), renderables);
    EXPECT_EQ(std::distance(list3.begin(), list3.end()), renderables.size());
    EXPECT_EQ(std::distance(list3.additional_layers_begin(), list3.end()), 0);
    EXPECT_EQ(std::distance(list3.begin(), list3.additional_layers_begin()), renderables.size());
}

TEST_F(LayerListTest, keeps_track_of_needs_commit)
{
    size_t fb_target_size{1};
    mga::LayerList list(layer_adapter, renderables);

    for(auto it = list.begin(); it != list.additional_layers_begin(); it++)
        EXPECT_TRUE(it->needs_commit);
    for(auto it = list.additional_layers_begin(); it != list.end(); it++)
        EXPECT_FALSE(it->needs_commit);

    mg::RenderableList list2{
        std::make_shared<mtd::StubRenderable>(buffer1),
        std::make_shared<mtd::StubRenderable>(buffer2),
        std::make_shared<mtd::StubRenderable>()
    };
    list.update_list(list2);

    //here, all should be needs_commit because they were all HWC_FRAMEBUFFER 
    for(auto it = list.begin(); it != list.additional_layers_begin(); it++)
        EXPECT_TRUE(it->needs_commit);

    ASSERT_THAT(list.native_list().lock()->numHwLayers, testing::Eq(list2.size() + fb_target_size));
    list.native_list().lock()->hwLayers[2].compositionType = HWC_OVERLAY;
    list.update_list(list2);

    auto i = 0;
    for(auto it = list.begin(); it != list.additional_layers_begin(); it++)
    {
        if (i == 2)
            EXPECT_FALSE(it->needs_commit);
        else
            EXPECT_TRUE(it->needs_commit);
        i++;
    }
    for(auto it = list.additional_layers_begin(); it != list.end(); it++)
        EXPECT_FALSE(it->needs_commit);
}

void fill_hwc_layer(
    hwc_layer_1_t& layer,
    hwc_rect_t* visible_rect,
    geom::Rectangle const& position,
    mg::Buffer const& buffer,
    int type, int flags)
{
    *visible_rect = {0, 0, buffer.size().width.as_int(), buffer.size().height.as_int()};
    layer.compositionType = type;
    layer.hints = 0;
    layer.flags = flags;
    layer.handle = buffer.native_buffer_handle()->handle();
    layer.transform = 0;
    layer.blending = HWC_BLENDING_NONE;
    layer.sourceCrop = *visible_rect;
    layer.displayFrame = {
        position.top_left.x.as_int(),
        position.top_left.y.as_int(),
        position.bottom_right().x.as_int(),
        position.bottom_right().y.as_int()
    };
    layer.visibleRegionScreen = {1, visible_rect};
    layer.acquireFenceFd = -1;
    layer.releaseFenceFd = -1;
    layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();
}

TEST_F(LayerListTest, setup_fb_hwc10)
{
    using namespace testing;
    geom::Size sz{44,22};
    auto mock_native_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>(sz);
    mtd::StubBuffer stub_buffer(mock_native_buffer, sz);
    hwc_layer_1_t layer;
    hwc_rect_t visible_rect;   
    geom::Rectangle const disp_frame{{0,0}, sz};
    fill_hwc_layer(layer, &visible_rect, disp_frame, stub_buffer, HWC_FRAMEBUFFER, HWC_SKIP_LAYER);

    mga::LayerList list(std::make_shared<mga::Hwc10Adapter>(), {});
    list.setup_fb(stub_buffer);

    auto l = list.native_list().lock();
    ASSERT_THAT(l->numHwLayers, Eq(1));
    EXPECT_THAT(l->hwLayers[l->numHwLayers-1], MatchesLegacyLayer(layer));
}

TEST_F(LayerListTest, setup_fb_without_skip)
{
    using namespace testing;
    geom::Size sz{44,22};
    auto mock_native_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>(sz);
    mtd::StubBuffer stub_buffer(mock_native_buffer, sz);
    hwc_layer_1_t layer;
    hwc_rect_t visible_rect;   
    geom::Rectangle const disp_frame{{0,0}, sz};
    fill_hwc_layer(layer, &visible_rect, disp_frame, stub_buffer, HWC_FRAMEBUFFER_TARGET, 0);

    mga::LayerList list(layer_adapter, renderables);
    list.setup_fb(stub_buffer);

    auto l = list.native_list().lock();
    ASSERT_THAT(l->numHwLayers, Eq(1 + renderables.size()));
    EXPECT_THAT(l->hwLayers[l->numHwLayers-1], MatchesLegacyLayer(layer));
}

TEST_F(LayerListTest, setup_fb_with_skip)
{
    using namespace testing;
    geom::Size sz{44,22};
    auto mock_native_buffer = std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>(sz);
    mtd::StubBuffer stub_buffer(mock_native_buffer, sz);
    hwc_layer_1_t layer;
    hwc_layer_1_t layer2;
    hwc_rect_t visible_rect;   
    geom::Rectangle const disp_frame{{0,0}, sz};
    fill_hwc_layer(layer, &visible_rect, disp_frame, stub_buffer, HWC_FRAMEBUFFER_TARGET, 0);
    fill_hwc_layer(layer2, &visible_rect, disp_frame, stub_buffer, HWC_FRAMEBUFFER, HWC_SKIP_LAYER);

    mga::LayerList list(layer_adapter, {});
    list.setup_fb(stub_buffer);
    auto l = list.native_list().lock();
    ASSERT_THAT(l->numHwLayers, Eq(2));
    EXPECT_THAT(l->hwLayers[l->numHwLayers-1], MatchesLegacyLayer(layer));
    EXPECT_THAT(l->hwLayers[l->numHwLayers-2], MatchesLegacyLayer(layer2));
}
