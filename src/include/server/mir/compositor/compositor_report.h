/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir/graphics/renderable.h"

namespace mir
{
namespace compositor
{

class CompositorReport
{
public:
    typedef const void* SubCompositorId;  // e.g. thread/display buffer ID
    virtual void added_display(int width, int height, int x, int y, SubCompositorId id) = 0;
    virtual void began_frame(SubCompositorId id) = 0;
    virtual void renderables_in_frame(SubCompositorId id, graphics::RenderableList const& renderables) = 0;
    virtual void rendered_frame(SubCompositorId id) = 0;
    virtual void finished_frame(SubCompositorId id) = 0;
    virtual void started() = 0;
    virtual void stopped() = 0;
    virtual void scheduled() = 0;
protected:
    CompositorReport() = default;
    virtual ~CompositorReport() = default;
    CompositorReport(CompositorReport const&) = delete;
    CompositorReport& operator=(CompositorReport const&) = delete;
};

} // namespace compositor
} // namespace mir

#endif // MIR_COMPOSITOR_COMPOSITOR_REPORT_H_
