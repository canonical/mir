/*
 * Copyright © Canonical Ltd.
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

#ifndef MIR_GRAPHICS_GBM_GBM_FORMAT_CONVERSIONS_H_
#define MIR_GRAPHICS_GBM_GBM_FORMAT_CONVERSIONS_H_

#include <mir_toolkit/common.h>
#include <stdint.h>
#include <limits>

namespace mir
{
namespace graphics
{
namespace atomic
{
enum : uint32_t { invalid_atomic_format = std::numeric_limits<uint32_t>::max() };

}
}
}
#endif /* MIR_GRAPHICS_GBM_GBM_FORMAT_CONVERSIONS_H_ */
