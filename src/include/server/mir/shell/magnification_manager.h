/*
* Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SHELL_MAGNIFICATION_MANAGER_H
#define MIR_SHELL_MAGNIFICATION_MANAGER_H

#include "mir/geometry/size.h"

namespace mir
{
namespace shell
{
class MagnificationManager
{
public:
    virtual ~MagnificationManager() = default;
    virtual void magnification(float magnification) = 0;
    virtual float magnification() const = 0;
    virtual bool enabled(bool enabled) = 0;
    virtual void size(geometry::Size const& size) = 0;
    virtual geometry::Size size() const = 0;
};
}
}

#endif //MIR_SHELL_MAGNIFICATION_MANAGER_H
