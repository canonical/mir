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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_CLIENT_PERF_REPORT_H_
#define MIR_CLIENT_PERF_REPORT_H_

namespace mir
{
namespace client
{

class PerfReport
{
public:
    PerfReport() = default;
    virtual void name_surface(char const*) = 0;
    virtual void begin_frame(int buffer_id) = 0;
    virtual void end_frame(int buffer_id) = 0;
protected:
    virtual ~PerfReport() = default;
    PerfReport(PerfReport const&) = delete;
    PerfReport& operator=(PerfReport const&) = delete;
};

class NullPerfReport : public PerfReport
{
public:
    virtual void name_surface(char const*) override {}
    void begin_frame(int) override {}
    void end_frame(int) override {}
};

} // namespace client
} // namespace mir

#endif // MIR_CLIENT_PERF_REPORT_H_
