/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_FRACTIONAL_SCALE_V1_H
#define MIR_FRONTEND_FRACTIONAL_SCALE_V1_H

#include "fractional-scale-v1_wrapper.h"

namespace mir
{
namespace frontend
{
auto create_fractional_scale_v1() -> std::shared_ptr<wayland::FractionalScaleManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_FRACTIONAL_SCALE_V1_H
