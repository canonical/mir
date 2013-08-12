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
 *
 * TODO: Un-inline this
 */

#ifndef MIR_COMPOSITOR_BYPASS_H_
#define MIR_COMPOSITOR_BYPASS_H_

#include "mir/graphics/display_buffer.h"
#include "mir/compositor/scene.h"

namespace mir
{
namespace compositor
{

class BypassFilter : public FilterForScene
{
public:
    BypassFilter(const graphics::DisplayBuffer &display_buffer)
    {
        const mir::geometry::Rectangle &rect = display_buffer.view_area();
        int width = rect.size.width.as_int();
        int height = rect.size.height.as_int();

        /*
         * For a surface to exactly fit the display_buffer, its transformation
         * will look exactly like this:
         */
        fullscreen[0][0] = width;
        fullscreen[0][1] = 0.0f;
        fullscreen[0][2] = 0.0f;
        fullscreen[0][3] = 0.0f;

        fullscreen[1][0] = 0.0f;
        fullscreen[1][1] = height;
        fullscreen[1][2] = 0.0f;
        fullscreen[1][3] = 0.0f;

        fullscreen[2][0] = 0.0f;
        fullscreen[2][1] = 0.0f;
        fullscreen[2][2] = 0.0f;
        fullscreen[2][3] = 0.0f;

        fullscreen[3][0] = rect.top_left.x.as_int() + width / 2;
        fullscreen[3][1] = rect.top_left.y.as_int() + height / 2;
        fullscreen[3][2] = 0.0f;
        fullscreen[3][3] = 1.0f;
    }

    bool operator()(CompositingCriteria const& criteria)
    {
        topmost_fits = criteria.transformation() == fullscreen;
        return topmost_fits;
    }

    bool fullscreen_on_top() const
    {
        return topmost_fits;
    }

private:
    bool topmost_fits = false;
    glm::mat4 fullscreen;
};

class BypassMatch : public OperatorForScene
{
public:
    void operator()(CompositingCriteria const&,
                    mir::surfaces::BufferStream& stream)
    {
        latest = &stream;
    }

    mir::surfaces::BufferStream *topmost_fullscreen() const
    {
        return latest;
    }

private:
    mir::surfaces::BufferStream *latest = nullptr;
};

} // namespace compositor
} // namespace mir

#endif // MIR_COMPOSITOR_BYPASS_H_
