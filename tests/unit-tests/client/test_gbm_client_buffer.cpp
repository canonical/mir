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

#include "mir_client/mir_client_library.h"
#include "mir_client/gbm/gbm_client_buffer_depository.h"
#include "mir_client/gbm/gbm_client_buffer.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mclg=mir::client::gbm;

struct MirGBMBufferTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        stride = geom::Stride(66);
        pf = geom::PixelFormat::rgba_8888;
        size = geom::Size{width, height};

        package = std::make_shared<MirBufferPackage>();
        package->stride = stride.as_uint32_t();
        package_copy = std::make_shared<MirBufferPackage>(*package.get());
    }
    geom::Width width;
    geom::Height height;
    geom::Stride stride;
    geom::PixelFormat pf;
    geom::Size size;

    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MirBufferPackage> package_copy;

};

TEST_F(MirGBMBufferTest, width_and_height)
{
    using namespace testing;

    mclg::GBMClientBuffer buffer(std::move(package), size, pf);

    EXPECT_EQ(buffer.size().height, height); 
    EXPECT_EQ(buffer.size().width, width); 
    EXPECT_EQ(buffer.pixel_format(), pf); 
}

TEST_F(MirGBMBufferTest, buffer_returns_correct_stride)
{
    using namespace testing;

    mclg::GBMClientBuffer buffer(std::move(package), size, pf);

    EXPECT_EQ(buffer.stride(), stride);
}

TEST_F(MirGBMBufferTest, buffer_returns_set_package)
{
    using namespace testing;

    mclg::GBMClientBuffer buffer(std::move(package), size, pf);

    auto package_return = buffer.get_buffer_package();
    EXPECT_EQ(package_return->data_items, package_copy->data_items);
    EXPECT_EQ(package_return->fd_items, package_copy->fd_items);
    EXPECT_EQ(package_return->stride, package_copy->stride);
    for(auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->data[i], package_copy->data[i]);
    for(auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->fd[i], package_copy->fd[i]);
}

