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
#include "src/platform/graphics/android/hwc_layerlist.h"
#include "hwc_struct_helpers.h"
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;

namespace
{
struct LayerListTest
{
    LayerListTest()
        : renderables{std::make_shared<mtd::StubRenderable>(),
                      std::make_shared<mtd::StubRenderable>(),
                      std::make_shared<mtd::StubRenderable>()}
    {}

    std::list<std::shared_ptr<mg::Renderable>> renderables;
};
}

TEST_F(LayerListTest, list_defaults)
{
    mga::LayerList layerlist{0};

    auto list = layerlist.native_list().lock();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    EXPECT_EQ(layerlist.begin(), layerlist.end());
}

TEST_F(LayerListTest, list_defaults)
{
    mga::LayerList layerlist{0};
    size_t additional_layers = 2;
    auto changed = update_list_and_check_if_changed(renderables, additional_layers);

    auto additional_ct = 0;
    auto composed_ct = 0; 
    auto in_additional_layers = false;
    for(auto it = layerlist.begin(); it != layerlist.end(); it++)
    {
        if (it == layerlist.last_composition_layer())
            in_additional_layers = true;

        if (in_additional_layers)
            additional_ct++;
        else
            composed_ct++;
    }

    EXPECT_EQ(composed_ct, renderables.size());
    EXPECT_EQ(addional_ct, additional_layers);
}

/* Tests without HWC_FRAMEBUFFER_TARGET */
/* an empty list should have a skipped layer. This will force a GL render on hwc set */
TEST_F(LayerListTest, hwc10_list_defaults)
{
    mga::LayerList layerlist;

    hwc_layer_1_t skip_layer;
    hwc_rect_t empty_region{0,0,0,0};

    skip_layer.compositionType = HWC_FRAMEBUFFER;
    skip_layer.hints = 0;
    skip_layer.flags = HWC_SKIP_LAYER;
    skip_layer.handle = 0;
    skip_layer.transform = 0;
    skip_layer.blending = HWC_BLENDING_NONE;
    skip_layer.sourceCrop = empty_region;
    skip_layer.displayFrame = empty_region;
    skip_layer.visibleRegionScreen = {1, &empty_region};
    skip_layer.acquireFenceFd = -1;
    skip_layer.releaseFenceFd = -1;

    auto list = layerlist.native_list().lock();
    ASSERT_EQ(1, list->numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
}
