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

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace
{
struct LayerListTest : public testing::Test
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
    mga::LayerList layerlist{{}, 0};

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
    mga::LayerList list(renderables, additional_layers);
    EXPECT_EQ(std::distance(list.begin(), list.end()), additional_layers + renderables.size());
    EXPECT_EQ(std::distance(list.additional_layers_begin(), list.end()), additional_layers);
    EXPECT_EQ(std::distance(list.begin(), list.additional_layers_begin()), renderables.size());

    mga::LayerList list2({}, additional_layers);
    EXPECT_EQ(std::distance(list2.begin(), list2.end()), additional_layers);
    EXPECT_EQ(std::distance(list2.additional_layers_begin(), list2.end()), additional_layers);
    EXPECT_EQ(std::distance(list2.begin(), list2.additional_layers_begin()), 0);

    mga::LayerList list3(renderables, 0);
    EXPECT_EQ(std::distance(list3.begin(), list3.end()), renderables.size());
    EXPECT_EQ(std::distance(list3.additional_layers_begin(), list3.end()), 0);
    EXPECT_EQ(std::distance(list3.begin(), list3.additional_layers_begin()), renderables.size());
}
