/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SHELL_SURFACE_ID_H_
#define MIR_SHELL_SURFACE_ID_H_

#include "mir/int_wrapper.h"

namespace mir
{
namespace shell
{
typedef IntWrapper<IntWrapperTypeTag::SessionsSurfaceId> SurfaceId;
}
} // namespace mir

#endif // MIR_SHELL_SURFACE_ID_H
