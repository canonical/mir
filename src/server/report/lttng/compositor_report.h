/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_REPORT_LTTNG_COMPOSITOR_REPORT_H_
#define MIR_REPORT_LTTNG_COMPOSITOR_REPORT_H_

#include "server_tracepoint_provider.h"

#include "mir/compositor/compositor_report.h"

namespace mir
{
namespace report
{
namespace lttng
{

class CompositorReport : public compositor::CompositorReport
{
public:
    CompositorReport() = default;
    virtual ~CompositorReport() = default;
    void added_display(int width, int height, int x, int y, SubCompositorId id) override;
    void began_frame(SubCompositorId id) override;
    void renderables_in_frame(SubCompositorId id, graphics::RenderableList const& renderables) override;
    void rendered_frame(SubCompositorId id) override;
    void finished_frame(SubCompositorId id) override;
    void started() override;
    void stopped() override;
    void scheduled() override;
private:
    ServerTracepointProvider tp_provider;
};

} // namespace lttng
} // namespace report
} // namespace mir

#endif // MIR_REPORT_LTTNG_COMPOSITOR_REPORT_H_
