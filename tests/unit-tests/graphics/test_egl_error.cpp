/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/graphics/egl_error.h"
#include "mir/test/doubles/mock_egl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{

std::string to_hex_string(int n)
{
    std::stringstream ss;
    ss << std::showbase << std::hex << n;
    return ss.str();
}

struct EGLErrorTest : ::testing::Test
{
    testing::NiceMock<mtd::MockEGL> mock_egl;

    std::string const user_message{"Something failed"};
    std::string const egl_error_code_str{"EGL_BAD_MATCH"};
    int const egl_error_code{EGL_BAD_MATCH};
};

}

TEST_F(EGLErrorTest, produces_correct_error_code)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetError())
        .WillOnce(Return(egl_error_code));

    std::error_code ec;

    try
    {
        throw mg::egl_error("");
    }
    catch (std::system_error const& error)
    {
        ec = error.code();
    }

    EXPECT_THAT(ec.value(), Eq(egl_error_code));
}

TEST_F(EGLErrorTest, produces_message_with_user_string_and_error_code_for_known_error)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetError())
        .WillOnce(Return(egl_error_code));

    std::string error_message;

    try
    {
        throw mg::egl_error(user_message);
    }
    catch (std::system_error const& error)
    {
        error_message = error.what();
    }

    EXPECT_THAT(error_message, HasSubstr(user_message));
    EXPECT_THAT(error_message, HasSubstr(egl_error_code_str));
    EXPECT_THAT(error_message, HasSubstr(to_hex_string(egl_error_code)));
}

TEST_F(EGLErrorTest, produces_message_with_user_string_and_error_code_for_unknown_error)
{
    using namespace testing;

    int const unknown_egl_error_code{0x131313};

    EXPECT_CALL(mock_egl, eglGetError())
        .WillOnce(Return(unknown_egl_error_code));

    std::string error_message;

    try
    {
        throw mg::egl_error(user_message);
    }
    catch (std::system_error const& error)
    {
        error_message = error.what();
    }

    EXPECT_THAT(error_message, HasSubstr(user_message));
    EXPECT_THAT(error_message, HasSubstr(to_hex_string(unknown_egl_error_code)));
}
