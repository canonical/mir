/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/shared_library_prober.h"

#include <unistd.h>
#include <libgen.h>

#include <system_error>
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <gtest/gtest.h>

namespace
{
std::string binary_path()
{
    char buf[1024];
    auto tmp = readlink("/proc/self/exe", buf, sizeof buf);
    if (tmp < 0)
        BOOST_THROW_EXCEPTION(boost::enable_error_info(
                                  std::runtime_error("Failed to find our executable path"))
                              << boost::errinfo_errno(errno));
    if (tmp > static_cast<ssize_t>(sizeof(buf) - 1))
        BOOST_THROW_EXCEPTION(std::runtime_error("Path to executable is too long!"));
    buf[tmp] = '\0';
    return dirname(buf);
}

class SharedLibraryProber : public testing::Test
{
public:
    SharedLibraryProber()
        : library_path{binary_path() + "/test_data"}
    {
    }

    std::string const library_path;
};
}

TEST_F(SharedLibraryProber, ReturnsNonEmptyListForPathContainingLibraries)
{
    auto libraries = mir::libraries_for_path(library_path);
    EXPECT_GE(libraries.size(), 1);
}

TEST_F(SharedLibraryProber, RaisesExceptionForNonexistentPath)
{
    EXPECT_THROW(mir::libraries_for_path("/a/path/that/certainly/doesnt/exist"),
                 std::system_error);
}

TEST_F(SharedLibraryProber, NonExistentPathRaisesENOENTError)
{
    try
    {
        mir::libraries_for_path("/a/path/that/certainly/doesnt/exist");
    }
    catch (std::system_error &err)
    {
        EXPECT_EQ(err.code(), std::error_code(ENOENT, std::system_category()));
    }
}
