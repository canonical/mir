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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_SURFACE_TARGET_H_
#define MIR_INPUT_SURFACE_TARGET_H_

#include "mir/geometry/size.h"
#include "mir/geometry/point.h"

#include <string>

namespace mir
{
namespace input
{

class SurfaceTarget
{
public:
    virtual ~SurfaceTarget() {}

    virtual geometry::Point top_left() const = 0;
    virtual geometry::Size size() const = 0;
    virtual std::string const& name() const = 0;

    virtual int server_input_fd() const = 0;

protected:
    SurfaceTarget() = default;
    SurfaceTarget(SurfaceTarget const&) = delete;
    SurfaceTarget& operator=(SurfaceTarget const&) = delete;
};

}
} // namespace mir

#endif // MIR_INPUT_SURFACE_TARGET_H_
