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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_COMPOSITOR_OCCLUSION_H_
#define MIR_COMPOSITOR_OCCLUSION_H_

#include "mir/compositor/scene.h"

namespace mir
{
namespace compositor
{

SceneElementSequence filter_occlusions_from(SceneElementSequence& list, geometry::Rectangle const& area);

} // namespace compositor
} // namespace mir

#endif // MIR_COMPOSITOR_OCCLUSION_H_
