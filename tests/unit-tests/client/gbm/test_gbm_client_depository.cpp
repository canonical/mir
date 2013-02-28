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

#include "mir_toolkit/mir_client_library.h"
#include "src/client/gbm/gbm_client_buffer_depository.h"
#include "src/client/gbm/gbm_client_buffer.h"
#include "mock_drm_fd_handler.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mclg=mir::client::gbm;

struct MirGBMBufferDepositoryTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        pf = geom::PixelFormat::abgr_8888;
        size = geom::Size{width, height};

        drm_fd_handler = std::make_shared<testing::NiceMock<mclg::MockDRMFDHandler>>();
        package = std::make_shared<MirBufferPackage>();
    }
    geom::Width width;
    geom::Height height;
    geom::PixelFormat pf;
    geom::Size size;

    std::shared_ptr<testing::NiceMock<mclg::MockDRMFDHandler>> drm_fd_handler;
    std::shared_ptr<MirBufferPackage> package;

};

TEST_F(MirGBMBufferDepositoryTest, depository_sets_width_and_height)
{
    using namespace testing;

    mclg::GBMClientBufferDepository depository{drm_fd_handler};

    depository.deposit_package(std::move(package), 8, size, pf);
    auto buffer = depository.access_buffer(8);

    EXPECT_EQ(buffer->size().height, height);
    EXPECT_EQ(buffer->size().width, width);
    EXPECT_EQ(buffer->pixel_format(), pf);
}

TEST_F(MirGBMBufferDepositoryTest, depository_new_deposit_changes_buffer )
{
    using namespace testing;

    mclg::GBMClientBufferDepository depository{drm_fd_handler};

    depository.deposit_package(std::move(package), 8, size, pf);
    auto buffer1 = depository.access_buffer(8);

    depository.deposit_package(std::move(package), 8, size, pf);
    auto buffer2 = depository.access_buffer(8);

    EXPECT_NE(buffer1, buffer2);
}

