/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/shared_library_prober.h"
#include "mir/shared_library.h"

#include <system_error>
#include <boost/filesystem.hpp>

std::vector<std::shared_ptr<mir::SharedLibrary>> mir::libraries_for_path(std::string const& path)
{
    // We use the error_code overload because we want to throw a std::system_error
    boost::system::error_code ec;
    boost::filesystem::directory_iterator iterator{path, ec};
    if (ec)
    {
        // *Of course* there's no good way to go from a boost::error_code to a std::error_code
        if (ec.category() == boost::system::system_category())
        {
            throw std::system_error{ec.value(), std::system_category()};
        }
        else
        {
            throw std::runtime_error{"Boost error from unknown category"};
        }
    }
    return std::vector<std::shared_ptr<mir::SharedLibrary>>{std::make_shared<mir::SharedLibrary>("/usr/lib/libacl.so")};
}
