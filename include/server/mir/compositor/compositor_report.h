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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_COMPOSITOR_COMPOSITOR_REPORT_H_
#define MIR_COMPOSITOR_COMPOSITOR_REPORT_H_

namespace mir
{
namespace compositor
{

class CompositorReport
{
public:
    typedef void* Id;  // Some unique value to distinguish the threads
    virtual void begin_frame(Id id = nullptr) = 0;
    virtual void end_frame(Id id = nullptr) = 0;
    virtual void start_compositor() = 0;
    virtual void stop_compositor() = 0;
protected:
    CompositorReport() = default;
    virtual ~CompositorReport() = default;
    CompositorReport(CompositorReport const&) = delete;
    CompositorReport& operator=(CompositorReport const&) = delete;
};

class NullCompositorReport : public CompositorReport
{
public:
    void begin_frame(Id id);
    void end_frame(Id id);
    virtual void start_compositor();
    virtual void stop_compositor();
};

} // namespace compositor
} // namespace mir

#endif // MIR_COMPOSITOR_COMPOSITOR_REPORT_H_
