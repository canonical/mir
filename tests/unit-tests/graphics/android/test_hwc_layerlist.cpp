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

#include "src/server/graphics/android/hwc_layerlist.h"
#include "mir_test_doubles/mock_buffer.h"
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
//MATCHER



class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        mock_buffer = std::make_shared<mtd::MockBuffer>();
    }

    std::shared_ptr<mtd::MockBuffer> mock_buffer;
};

TEST_F(HWCLayerListTest, empty_list)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    auto list = layerlist.native_list(); 
    EXPECT_EQ(0u, list.size());
}

TEST_F(HWCLayerListTest, set_fb_target)
{
    using namespace testing;

    mga::HWCLayerList layerlist;

    layerlist.set_fb_target(mock_buffer);

    auto list = layerlist.native_list(); 
    EXPECT_EQ(1u, list.size());
}

#if 0
TEST_F(HWCDevice, set_fb_target_2x)
{
    using namespace testing;

    mga::HWC11LayerList layerlist;

    layerlist.set_fb_target(android_buffer);
    auto list = layerlist.native_list();

}
#endif
