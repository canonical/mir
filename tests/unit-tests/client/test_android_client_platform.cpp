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

#include "mir_client/mir_client_library.h"
#include "mir_client/client_platform.h"
#include "mir_client/client_context.h"

#include <EGL/egl.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl = mir::client;

struct MockClientContext : public mcl::ClientContext
{
    MockClientContext()
        : connection{reinterpret_cast<MirConnection*>(0xabcdef)}
    {
        using namespace testing;

        ON_CALL(*this, mir_connection()).WillByDefault(Return(connection));
        EXPECT_CALL(*this, mir_connection()).Times(AtLeast(0));
        EXPECT_CALL(*this, populate(_)).Times(AtLeast(0));
    }

    MirConnection* connection;

    MOCK_METHOD0(mir_connection, MirConnection*());
    MOCK_METHOD1(populate, void(MirPlatformPackage&));
};

TEST(AndroidClientPlatformTest, egl_native_display_is_egl_default_display)
{
    MockClientContext context;
    auto platform = mcl::create_client_platform(&context);
    auto native_display = platform->create_egl_native_display();
    EGLNativeDisplayType egl_native_display = native_display->get_egl_native_display();
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, egl_native_display);
}
