/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_EVENT_HELPERS_H_
#define MIR_COMMON_EVENT_HELPERS_H_

#include "event.h"

namespace mir
{
namespace events
{
void set_local_position(MirEvent& event, mir::geometry::DisplacementF const& offset_from_global);
void scale_local_position(MirEvent& event, float scale);
}
}

#endif // MIR_COMMON_EVENT_HELPERS_H_
