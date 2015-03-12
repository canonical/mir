/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_LTTNG_PERF_REPORT_H_
#define MIR_CLIENT_LTTNG_PERF_REPORT_H_

#include "../perf_report.h"
#include "client_tracepoint_provider.h"

namespace mir
{
namespace client
{
namespace lttng
{
class PerfReport : public client::PerfReport
{
public:
    void name_surface(char const*) override;
    void begin_frame(int buffer_id) override;
    void end_frame(int buffer_id) override;
private:
    ClientTracepointProvider tp_provider;
};
}
}
}
#endif /* MIR_CLIENT_LTTNG_PERF_REPORT_H_ */
