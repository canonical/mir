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

#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/drm_authenticator.h"
#include "src/server/graphics/gbm/gbm_platform.h"
#include "src/server/graphics/gbm/internal_client.h"
#include "mir_test_doubles/null_virtual_terminal.h"
#include "mir_test_doubles/stub_surface.h"

#include "mir/graphics/null_display_report.h"

#include <gtest/gtest.h>

#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mtd = mir::test::doubles;

namespace
{

class GBMGraphicsPlatform : public ::testing::Test
{
public:
    void SetUp()
    {
        ::testing::Mock::VerifyAndClearExpectations(&mock_drm);
        ::testing::Mock::VerifyAndClearExpectations(&mock_gbm);
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
        return std::make_shared<mgg::GBMPlatform>(
            std::make_shared<mg::NullDisplayReport>(),
            std::make_shared<mtd::NullVirtualTerminal>());
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
};
}

TEST_F(GBMGraphicsPlatform, get_ipc_package)
{
    using namespace testing;
    const int auth_fd{66};

    /* First time for master DRM fd, second for authenticated fd */
    EXPECT_CALL(mock_drm, drmOpen(_,_))
        .Times(2)
        .WillOnce(Return(mock_drm.fake_drm.fd()))
        .WillOnce(Return(auth_fd));

    /* Expect proper authorization */
    EXPECT_CALL(mock_drm, drmGetMagic(auth_fd,_));
    EXPECT_CALL(mock_drm, drmAuthMagic(mock_drm.fake_drm.fd(),_));

    EXPECT_CALL(mock_drm, drmClose(mock_drm.fake_drm.fd()));

    /* Expect authenticated fd to be closed when package is destroyed */
    EXPECT_CALL(mock_drm, drmClose(auth_fd));

    EXPECT_NO_THROW (
        auto platform = create_platform();
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
        auto platform = create_platform();
    } catch(...)
    {
        return;
    }

    FAIL() << "Expected an exception to be thrown.";
}

TEST_F(GBMGraphicsPlatform, drm_auth_magic_calls_drm_function_correctly)
{
    using namespace testing;

    drm_magic_t const magic{0x10111213};

    EXPECT_CALL(mock_drm, drmAuthMagic(mock_drm.fake_drm.fd(),magic))
        .WillOnce(Return(0));

    auto platform = create_platform();
    auto authenticator = std::dynamic_pointer_cast<mg::DRMAuthenticator>(platform);
    authenticator->drm_auth_magic(magic);
}

TEST_F(GBMGraphicsPlatform, drm_auth_magic_throws_if_drm_function_fails)
{
    using namespace testing;

    drm_magic_t const magic{0x10111213};

    EXPECT_CALL(mock_drm, drmAuthMagic(mock_drm.fake_drm.fd(),magic))
        .WillOnce(Return(-1));

    auto platform = create_platform();
    auto authenticator = std::dynamic_pointer_cast<mg::DRMAuthenticator>(platform);

    EXPECT_THROW({
        authenticator->drm_auth_magic(magic);
    }, std::runtime_error);
}

/* TODO: this function is a bit fragile because libmirserver and libmirclient both have very different
 *       implementations and both have symbols for it. If the linking order of the test changes,
 *       specifically, if mir_egl_mesa_display_is_valid resolves into libmirclient, then this test will break. 
 */
TEST_F(GBMGraphicsPlatform, platform_provides_validation_of_display_for_internal_clients)
{
    auto stub_surface = std::make_shared<mtd::StubSurface>();
    MirMesaEGLNativeDisplay* native_display = nullptr;
    EXPECT_EQ(0, mir_server_internal_display_is_valid(native_display));
    {
        auto platform = create_platform();
        auto client = platform->create_internal_client(stub_surface);
        native_display = reinterpret_cast<MirMesaEGLNativeDisplay*>(client->egl_native_display());
        EXPECT_EQ(1, mir_server_internal_display_is_valid(native_display));
    }
    EXPECT_EQ(0, mir_server_internal_display_is_valid(native_display));
}
