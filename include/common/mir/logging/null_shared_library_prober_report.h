/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_LOGGING_NULL_SHARED_LIBRARY_PROBER_REPORT_H_
#define MIR_LOGGING_NULL_SHARED_LIBRARY_PROBER_REPORT_H_

#include <mir/shared_library_prober_report.h>

namespace mir
{
namespace logging
{

class NullSharedLibraryProberReport : public mir::SharedLibraryProberReport
{
public:
    void probing_path(std::filesystem::path const&) override
    {
    }
    void probing_failed(std::filesystem::path const& /*path*/, std::exception const& /*error*/) override
    {
    }
    void loading_library(std::filesystem::path const& /*filename*/) override
    {
    }
    void loading_failed(std::filesystem::path const& /*filename*/, std::exception const& /*error*/) override
    {
    }
};

}
}

#endif /* MIR_LOGGING_NULL_SHARED_LIBRARY_PROBER_REPORT_H_ */
