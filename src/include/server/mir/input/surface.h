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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_INPUT_SURFACE_H_
#define MIR_INPUT_SURFACE_H_

#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"
#include "mir/input/input_reception_mode.h"

#include "mir_toolkit/event.h"

#include <string>
#include <memory>

namespace mir
{
namespace graphics
{
class CursorImage;
}

namespace scene
{
class SurfaceObserver;
}

namespace input
{

class Surface
{
public:
    virtual std::string name() const = 0;
    virtual geometry::Rectangle input_bounds() const = 0;
    virtual bool input_area_contains(geometry::Point const& point) const = 0;
    virtual std::shared_ptr<graphics::CursorImage> cursor_image() const = 0;
    virtual InputReceptionMode reception_mode() const = 0;
    virtual void consume(MirEvent const* event) = 0;

protected:
    Surface() = default;
    virtual ~Surface() = default;
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface& ) = delete;
};

}
}
#endif /* MIR_INPUT_SURFACE_H_ */
