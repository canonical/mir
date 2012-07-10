/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SURFACES_SURFACE_RENDERER_H_
#define MIR_SURFACES_SURFACE_RENDERER_H_

#include <memory>

namespace mir
{
namespace surfaces
{

class Surface;

class SurfaceRenderer
{
public:
    virtual ~SurfaceRenderer() {}

    virtual void render(const std::shared_ptr<Surface>& surface) = 0;
    
protected:
    SurfaceRenderer() = default;
    SurfaceRenderer(const SurfaceRenderer&) = delete;
    SurfaceRenderer& operator=(const SurfaceRenderer&) = delete;
};

}
}

#endif // MIR_SURFACES_SURFACE_RENDERER_H_
