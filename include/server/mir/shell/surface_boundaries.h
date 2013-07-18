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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SHELL_SURFACE_BOUNDARIES_H_
#define MIR_SHELL_SURFACE_BOUNDARIES_H_

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace shell
{

/**
 * Interface to the boundaries of valid surface placements.
 */
class SurfaceBoundaries
{
public:
    virtual ~SurfaceBoundaries() = default;

    /**
     * Clips a rectangle to the screen it is in.
     *
     * @param [in,out] rect the rectangle to clip
     */
    virtual void clip_to_screen(geometry::Rectangle& rect) = 0;

    /**
     * Makes a rectangle fullscreen in the screen it is in.
     *
     * @param [in,out] rect the rectangle to make fullscreen
     */
    virtual void make_fullscreen(geometry::Rectangle& rect) = 0;

protected:
    SurfaceBoundaries() = default;
    SurfaceBoundaries(SurfaceBoundaries const&) = delete;
    SurfaceBoundaries& operator=(SurfaceBoundaries const&) = delete;
};

}
}

#endif /* MIR_SHELL_SURFACE_BOUNDARIES_H_ */
