/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/shared_library_prober.h"
#include "mir/shared_library.h"

#include <boost/throw_exception.hpp>

#include <system_error>
#include <cstring>
#include <algorithm>

namespace
{
// Libraries can be of the form libname.so(.X.Y)
bool path_has_library_extension(std::filesystem::path const& path)
{
    return path.extension().string() == ".so" ||
           path.string().find(".so.") != std::string::npos;
}

}

namespace
{
bool greater_soname_version(std::filesystem::path const& lhs, std::filesystem::path const& rhs)
{
    auto lhbuf = strrchr(lhs.c_str(), '.');
    auto rhbuf = strrchr(rhs.c_str(), '.');

    if (!rhbuf) return lhbuf;
    if (!lhbuf) return false;

    return strtol(++lhbuf, 0, 0) > strtol(++rhbuf, 0, 0);
}
}

void mir::select_libraries_for_path(
    std::string const& path,
    std::function<Selection(std::shared_ptr<mir::SharedLibrary> const&)> const& selector,
    mir::SharedLibraryProberReport& report)
{
    report.probing_path(path);
    // We use the error_code overload because we want to report an error
    std::error_code ec;

    std::filesystem::directory_iterator iterator{path, ec};
    if (ec)
    {
        std::system_error error(ec.value(), std::system_category(), path);
        report.probing_failed(path, error);
        throw error;
    }

    std::vector<std::filesystem::path> libraries;
    for (; iterator != std::filesystem::directory_iterator() ; ++iterator)
    {
        if (path_has_library_extension(iterator->path()))
            libraries.push_back(iterator->path().string());
    }

    std::sort(libraries.begin(), libraries.end(), &greater_soname_version);

    for(auto& lib : libraries)
    {
        try
        {
            report.loading_library(lib);
            auto const shared_lib = std::make_shared<mir::SharedLibrary>(lib.string());

            if (selector(shared_lib) == Selection::quit)
                return;
        }
        catch (std::runtime_error const& err)
        {
            report.loading_failed(lib, err);
        }
    }
}

std::vector<std::shared_ptr<mir::SharedLibrary>>
mir::libraries_for_path(std::string const& path, mir::SharedLibraryProberReport& report)
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> result;

    select_libraries_for_path(
        path,
        [&](std::shared_ptr<mir::SharedLibrary> const& shared_lib)
            { result.push_back(shared_lib); return Selection::persist; },
        report);

    return result;
}
