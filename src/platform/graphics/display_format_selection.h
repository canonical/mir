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
 */

#ifndef MIR_GRAPHICS_DISPLAY_FORMAT_SELECTION_H_
#define MIR_GRAPHICS_DISPLAY_FORMAT_SELECTION_H_

namespace mir::graphics
{
class CPUAddressableDisplayAllocator;
class DRMFormat;

/**
 * Pick the format we would most like to render into for this display
 *
 * \throws  std::runtime_error if the allocator supports no format we can render into
 */
auto select_format_from(CPUAddressableDisplayAllocator const& allocator) -> DRMFormat;

}

#endif // MIR_GRAPHICS_DISPLAY_FORMAT_SELECTION_H_
