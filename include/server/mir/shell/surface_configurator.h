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

#ifndef MIR_SHELL_SURFACE_CONFIGURATOR_H_
#define MIR_SHELL_SURFACE_CONFIGURATOR_H_

#include "mir_toolkit/common.h"

#include <memory>

namespace mir
{

namespace shell
{
class Surface;

class SurfaceConfigurator
{
public:
    virtual ~SurfaceConfigurator() = default;
    
    /// Returns the selected value.
    virtual int select_attribute_value(Surface const& surface, MirSurfaceAttrib attrib,
                                       int requested_value) = 0;

    virtual void attribute_set(Surface const& surface, MirSurfaceAttrib attrib,
                               int new_value) = 0;

protected:
    SurfaceConfigurator() = default;
    SurfaceConfigurator(SurfaceConfigurator const&) = delete;
    SurfaceConfigurator& operator=(SurfaceConfigurator const&) = delete;
};

}
} // namespace mir

#endif // MIR_SHELL_SURFACE_CONFIGURATOR_H_
