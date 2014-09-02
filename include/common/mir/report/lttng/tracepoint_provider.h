/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_REPORT_LTTNG_TRACEPOINT_PROVIDER_H_
#define MIR_REPORT_LTTNG_TRACEPOINT_PROVIDER_H_

#include <string>

namespace mir
{
namespace report
{
namespace lttng
{

class TracepointProvider
{
public:
    TracepointProvider(std::string const& lib_name);
    ~TracepointProvider() noexcept;

private:
    TracepointProvider(TracepointProvider const&) = delete;
    TracepointProvider& operator=(TracepointProvider const&) = delete;

    void* lib;
};

}
}
}

#endif /* MIR_REPORT_LTTNG_TRACEPOINT_PROVIDER_H_ */
