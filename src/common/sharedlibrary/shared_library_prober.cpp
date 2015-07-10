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

namespace
{
std::error_code boost_to_std_error(boost::system::error_code const& ec)
{
    if (ec)
    {
        if (ec.category() != boost::system::system_category())
        {
            throw std::logic_error{"Boost error from unexpected category: " +
                                   ec.message()};
        }
        return std::error_code{ec.value(), std::system_category()};
    }
    return std::error_code{};
}

// Libraries can be of the form libname.so(.X.Y)
bool path_has_library_extension(boost::filesystem::path const& path)
{
    return path.extension().string() == ".so" ||
           path.string().find(".so.") != std::string::npos;
}

}

std::vector<std::shared_ptr<mir::SharedLibrary>>
mir::libraries_for_path(std::string const& path, mir::SharedLibraryProberReport& report)
{
    report.probing_path(path);
    // We use the error_code overload because we want to throw a std::system_error
    boost::system::error_code ec;

    boost::filesystem::directory_iterator iterator{path, ec};
    if (ec)
    {
        std::system_error error(boost_to_std_error(ec), path);
        report.probing_failed(path, error);
        throw error;
    }

    std::vector<std::shared_ptr<mir::SharedLibrary>> libraries;
    for (; iterator != boost::filesystem::directory_iterator() ; ++iterator)
    {
        if (path_has_library_extension(iterator->path()))
        {
            try
            {
                report.loading_library(iterator->path());
                libraries.emplace_back(std::make_shared<mir::SharedLibrary>(iterator->path().string()));
            }
            catch (std::runtime_error const& err)
            {
                report.loading_failed(iterator->path(), err);
            }
        }
    }
    return libraries;
}
