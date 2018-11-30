/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/platforms/eglstream-kms/server/utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mge = mir::graphics::eglstream;

using namespace testing;

namespace mir
{
namespace graphics
{
namespace eglstream
{
bool operator==(VersionInfo const& a, VersionInfo const& b)
{
    return a.major == b.major && a.minor == b.minor;
}
}
}
}

TEST(EGLStreamUtils, returns_empty_option_on_non_nvidia_version)
{
    EXPECT_THAT(
        mge::parse_nvidia_version("Not an nvidia version"),
        Eq(std::experimental::optional<mge::VersionInfo>{}));
}

TEST(EGLStreamUtils, returns_empty_option_on_empty_string)
{
    EXPECT_THAT(
        mge::parse_nvidia_version(""),
        Eq(std::experimental::optional<mge::VersionInfo>{}));
}

TEST(EGLStreamUtils, returns_nvidia_driver_version)
{
    auto const version = mge::parse_nvidia_version("4.5 NVIDIA 390.23");
    EXPECT_TRUE(!!version);
    EXPECT_THAT(*version, Eq(mge::VersionInfo { 390, 23 }));
}

