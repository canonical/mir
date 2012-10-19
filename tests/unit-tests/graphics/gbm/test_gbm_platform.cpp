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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/logging/dumb_console_logger.h"

#include "mock_drm.h"
#include "mock_gbm.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace mg = mir::graphics;
namespace ml = mir::logging;

namespace
{
class GBMGraphicsPlatform : public ::testing::Test
{
public:
    GBMGraphicsPlatform() : logger(std::make_shared<ml::DumbConsoleLogger>())
    {
    }

    void SetUp()
    {
        ::testing::Mock::VerifyAndClearExpectations(&mock_drm);
        ::testing::Mock::VerifyAndClearExpectations(&mock_gbm);
    }
    std::shared_ptr<ml::Logger> logger;
    ::testing::NiceMock<mg::gbm::MockDRM> mock_drm;
    ::testing::NiceMock<mg::gbm::MockGBM> mock_gbm;
};
}

TEST_F(GBMGraphicsPlatform, get_ipc_package)
{
    using namespace testing;
    const int auth_fd{66};

    /* First time for master DRM fd, second for authenticated fd */
    EXPECT_CALL(mock_drm, drmOpen(_,_))
        .Times(2)
        .WillOnce(Return(mock_drm.fake_drm.fd))
        .WillOnce(Return(auth_fd));

    /* Expect proper authorization */
    EXPECT_CALL(mock_drm, drmGetMagic(auth_fd,_));
    EXPECT_CALL(mock_drm, drmAuthMagic(mock_drm.fake_drm.fd,_));

    EXPECT_CALL(mock_drm, drmClose(mock_drm.fake_drm.fd));

    /* Expect authenticated fd to be closed when package is destroyed */
    EXPECT_CALL(mock_drm, drmClose(auth_fd));

    EXPECT_NO_THROW (
        auto platform = mg::create_platform();
        auto pkg = platform->get_ipc_package();

        ASSERT_TRUE(pkg.get());
        ASSERT_EQ(std::vector<int32_t>::size_type{1}, pkg->ipc_fds.size()); 
        ASSERT_EQ(auth_fd, pkg->ipc_fds[0]); 
    );
}

TEST_F(GBMGraphicsPlatform, a_failure_while_creating_a_platform_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_drm, drmOpen(_,_))
            .WillRepeatedly(Return(-1));

    try
    {
        auto platform = mg::create_platform();
    } catch(...)
    {
        return;
    }

    FAIL() << "Expected an exception to be thrown.";
}
