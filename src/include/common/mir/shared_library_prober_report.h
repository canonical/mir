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

#ifndef MIR_SHARED_LIBRARY_PROBER_REPORT_H_
#define MIR_SHARED_LIBRARY_PROBER_REPORT_H_

#include <filesystem>

namespace mir
{
class SharedLibraryProberReport
{
public:
    virtual ~SharedLibraryProberReport() = default;

    virtual void probing_path(std::filesystem::path const& path) = 0;
    virtual void probing_failed(std::filesystem::path const& path, std::exception const& error) = 0;
    virtual void loading_library(std::filesystem::path const& filename) = 0;
    virtual void loading_failed(std::filesystem::path const& filename, std::exception const& error) = 0;

protected:
    SharedLibraryProberReport() = default;
    SharedLibraryProberReport(SharedLibraryProberReport const&) = delete;
    SharedLibraryProberReport& operator=(SharedLibraryProberReport const&) = delete;
};
}


#endif /* MIR_SHARED_LIBRARY_PROBER_REPORT_H_ */
