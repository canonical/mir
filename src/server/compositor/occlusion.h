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
#include <vector>
#include <set>

namespace mir
{
namespace compositor
{

class OcclusionFilter : public FilterForScene
{
public:
    OcclusionFilter(const geometry::Rectangle &area);
    bool operator()(const graphics::Renderable &renderable) override;

private:
    const geometry::Rectangle &area;

    typedef std::vector<geometry::Rectangle> RectangleList;
    RectangleList coverage;
};

class OcclusionMatch : public OperatorForScene
{
public:
    void operator()(const graphics::Renderable &renderable) override;

    bool occluded(const graphics::Renderable &renderable) const;

private:
    typedef std::set<const graphics::Renderable*> RenderableSet;
    RenderableSet hidden;
};

} // namespace compositor
} // namespace mir

#endif // MIR_COMPOSITOR_OCCLUSION_H_
