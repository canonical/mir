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

#ifndef MIR_COMPOSITOR_OCCLUSION_H_
#define MIR_COMPOSITOR_OCCLUSION_H_

#include "mir/compositor/scene.h"
#include "glm/glm.hpp"
#include <vector>
#include <set>

namespace mir
{
namespace graphics
{
class DisplayBuffer;
}
namespace compositor
{
class CompositingCriteria;

class OcclusionFilter : public FilterForScene
{
public:
    OcclusionFilter(const graphics::DisplayBuffer &display_buffer);
    bool operator()(const CompositingCriteria &criteria) override;

private:
    const graphics::DisplayBuffer &display_buffer;

    typedef std::vector<geometry::Rectangle> RectangleList;
    RectangleList coverage;
};

class OcclusionMatch : public OperatorForScene
{
public:
    void operator()(const CompositingCriteria &,
                    surfaces::BufferStream &stream) override;

    bool occluded(const CompositingCriteria &criteria) const;

private:
    /*
     * This is realible, but not ideal -- using the criteria address as a
     * unique key for renderables. Ideally we should have access to a
     * Renderable interface that has a unique ID we can use. But this works
     * for now...
     */
    typedef std::set<const CompositingCriteria*> RenderableSet;
    RenderableSet hidden;
};

} // namespace compositor
} // namespace mir

#endif // MIR_COMPOSITOR_OCCLUSION_H_
