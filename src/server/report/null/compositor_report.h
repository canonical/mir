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

#ifndef MIR_REPORT_NULL_COMPOSITOR_REPORT_H_
#define MIR_REPORT_NULL_COMPOSITOR_REPORT_H_

#include "mir/compositor/compositor_report.h"

namespace mir
{
namespace report
{
namespace null
{

class CompositorReport : public compositor::CompositorReport
{
public:
    void added_display(int width, int height, int x, int y, SubCompositorId id) override;
    void began_frame(SubCompositorId id) override;
    void renderables_in_frame(SubCompositorId id, graphics::RenderableList const& renderables) override;
    void rendered_frame(SubCompositorId id) override;
    void finished_frame(SubCompositorId id) override;
    void started() override;
    void stopped() override;
    void scheduled() override;
};

} // namespace compositor
} // namespace report
} // namespace mir

#endif // MIR_REPORT_NULL_COMPOSITOR_REPORT_H_
