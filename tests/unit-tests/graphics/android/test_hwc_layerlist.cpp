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
#include "hwc_struct_helpers.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
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
    mga::LayerList layerlist{layer_adapter, {}, 0};

    auto list = layerlist.native_list().lock();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    EXPECT_EQ(layerlist.begin(), layerlist.end());
    EXPECT_EQ(layerlist.additional_layers_begin(), layerlist.end());
}

TEST_F(LayerListTest, list_iterators)
{
    size_t additional_layers = 2;
    mga::LayerList list(layer_adapter, renderables, additional_layers);
    EXPECT_EQ(std::distance(list.begin(), list.end()), additional_layers + renderables.size());
    EXPECT_EQ(std::distance(list.additional_layers_begin(), list.end()), additional_layers);
    EXPECT_EQ(std::distance(list.begin(), list.additional_layers_begin()), renderables.size());

    mga::LayerList list2(layer_adapter, {}, additional_layers);
    EXPECT_EQ(std::distance(list2.begin(), list2.end()), additional_layers);
    EXPECT_EQ(std::distance(list2.additional_layers_begin(), list2.end()), additional_layers);
    EXPECT_EQ(std::distance(list2.begin(), list2.additional_layers_begin()), 0);

    mga::LayerList list3(layer_adapter, renderables, 0);
    EXPECT_EQ(std::distance(list3.begin(), list3.end()), renderables.size());
    EXPECT_EQ(std::distance(list3.additional_layers_begin(), list3.end()), 0);
    EXPECT_EQ(std::distance(list3.begin(), list3.additional_layers_begin()), renderables.size());
}

TEST_F(LayerListTest, keeps_track_of_needs_commit)
{
    size_t additional_layers = 4;
    mga::LayerList list(layer_adapter, renderables, additional_layers);

    for(auto it = list.begin(); it != list.additional_layers_begin(); it++)
        EXPECT_TRUE(it->needs_commit);
    for(auto it = list.additional_layers_begin(); it != list.end(); it++)
        EXPECT_FALSE(it->needs_commit);

    mg::RenderableList list2{
        std::make_shared<mtd::StubRenderable>(buffer1),
        std::make_shared<mtd::StubRenderable>(buffer2),
        std::make_shared<mtd::StubRenderable>()
    };
    list.update_list(list2, additional_layers);

    //here, all should be needs_commit because they were all HWC_FRAMEBUFFER 
    for(auto it = list.begin(); it != list.additional_layers_begin(); it++)
        EXPECT_TRUE(it->needs_commit);

    ASSERT_THAT(list.native_list().lock()->numHwLayers, testing::Eq(list2.size() + additional_layers));
    list.native_list().lock()->hwLayers[2].compositionType = HWC_OVERLAY;
    list.update_list(list2, additional_layers);

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
