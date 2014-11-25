/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir/graphics/nested_context.h"
#include "src/platform/graphics/mesa/nested_authentication.h"
#include "mir_test_doubles/mock_drm.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockNestedContext : mg::NestedContext
{
    MOCK_METHOD0(platform_fd_items, std::vector<int>());
    MOCK_METHOD1(drm_auth_magic, void(int magic));
    MOCK_METHOD1(drm_set_gbm_device, void(struct gbm_device* dev));
};

struct NestedAuthentication : public ::testing::Test
{
    NestedAuthentication()
    {
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    MockNestedContext mock_nested_context;
    mgm::NestedAuthentication auth{mt::fake_shared(mock_nested_context)};
};
}

TEST_F(NestedAuthentication, uses_nested_context_to_authenticate)
{
    int magic{332211};
    EXPECT_CALL(mock_nested_context, drm_auth_magic(magic));
    auth.auth_magic(magic);
}

TEST_F(NestedAuthentication, no_drm_in_platform_package_throws)
{
    using namespace testing;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{}));
    EXPECT_THROW({
        auth.authenticated_fd();
    }, std::runtime_error);
}

TEST_F(NestedAuthentication, failure_in_getbusid_throws)
{
    using namespace testing;
    int stub_fd = 33;

    InSequence seq;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{stub_fd}));
    EXPECT_CALL(mock_drm, drmGetBusid(stub_fd))
        .WillOnce(Return(nullptr));

    EXPECT_THROW({
        auth.authenticated_fd();
    }, std::runtime_error);
}

TEST_F(NestedAuthentication, failure_in_getmagic_throws)
{
    using namespace testing;
    int stub_fd = 33;
    int auth_fd = 34;
    //this should get freed by the implementation
    auto fake_busid = static_cast<char*>(malloc(sizeof(char)));
    *fake_busid = 'a';
    int magic{332211};

    InSequence seq;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{stub_fd}));
    EXPECT_CALL(mock_drm, drmGetBusid(stub_fd))
        .WillOnce(Return(fake_busid));
    EXPECT_CALL(mock_drm, drmOpen(nullptr, fake_busid))
        .WillOnce(Return(auth_fd));
    EXPECT_CALL(mock_drm, drmGetMagic(auth_fd, _))
        .WillOnce(DoAll(SetArgPointee<1>(magic), Return(-1)));

    EXPECT_THROW({
        auth.authenticated_fd();
    }, std::runtime_error);
}

TEST_F(NestedAuthentication, creates_new_fd)
{
    using namespace testing;
    int stub_fd = 33;
    int auth_fd = 34;
    //this should get freed by the implementation
    auto fake_busid = static_cast<char*>(malloc(sizeof(char)));
    *fake_busid = 'a';
    int magic{332211};

    InSequence seq;
    EXPECT_CALL(mock_nested_context, platform_fd_items())
        .WillOnce(Return(std::vector<int>{stub_fd}));
    EXPECT_CALL(mock_drm, drmGetBusid(stub_fd))
        .WillOnce(Return(fake_busid));
    EXPECT_CALL(mock_drm, drmOpen(nullptr, fake_busid))
        .WillOnce(Return(auth_fd));
    EXPECT_CALL(mock_drm, drmGetMagic(auth_fd, _))
        .WillOnce(DoAll(SetArgPointee<1>(magic), Return(0)));
    EXPECT_CALL(mock_nested_context, drm_auth_magic(magic));

    auto fd = auth.authenticated_fd();
    EXPECT_THAT(fd, Eq(auth_fd));
}
