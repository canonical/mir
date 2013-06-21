/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shared_library.h"

#include <gtest/gtest.h>

#include <boost/exception/diagnostic_information.hpp>

#include <stdexcept>

namespace
{
class HasSubstring
{
public:
    HasSubstring(char const* substring) : substring(substring) {}

    friend::testing::AssertionResult operator,(std::string const& target, HasSubstring const& match)
    {
      if (std::string::npos != target.find(match.substring))
        return ::testing::AssertionSuccess();
      else
        return ::testing::AssertionFailure() <<
            "The target:\n\"" << target << "\"\n"
            "Does not contain:\n\"" << match.substring << "\"";
    }

private:
    char const* const substring;

    HasSubstring(HasSubstring const&) = delete;
    HasSubstring& operator=(HasSubstring const&) = delete;
};

}

TEST(SharedLibrary, load_nonexistent_library_fails)
{
    EXPECT_THROW({ mir::SharedLibrary nonexistent(__PRETTY_FUNCTION__); }, std::runtime_error);
}

TEST(SharedLibrary, load_nonexistent_library_fails_with_useful_info)
{
    char const* const nonexistent_library = "nonexistent_library";

    try
    {
        mir::SharedLibrary nonexistent(nonexistent_library);
    }
    catch (std::exception const& error)
    {
        auto info = boost::diagnostic_information(error);

        EXPECT_TRUE((info, HasSubstring("cannot open shared object"))) << "What went wrong";
        EXPECT_TRUE((info, HasSubstring(nonexistent_library))) << "Name of library";
    }
}

TEST(SharedLibrary, load_valid_library_works)
{
    mir::SharedLibrary existing("libmirplatformgraphics.so");
}


