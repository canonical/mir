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

#include "display_format_selection.h"

#include <mir/graphics/display_providers.h>
#include <mir/graphics/drm_formats.h>

#include <boost/throw_exception.hpp>
#include <drm_fourcc.h>
#include <optional>
#include <stdexcept>

namespace mg = mir::graphics;

auto mg::select_format_from(CPUAddressableDisplayAllocator const& allocator) -> mg::DRMFormat
{
    std::optional<mg::DRMFormat> best_format;
    for (auto const format : allocator.supported_formats())
    {
        switch(static_cast<uint32_t>(format))
        {
        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_XRGB8888:
            // ?RGB8888 is the easiest for us
            return format;
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_RGBX8888:
            // RGB?8888 requires an EGL extension, but is OK
            best_format = format;
            break;
        default:
            // We only care about the above two; include a default case to
            // make this clear.
            break;
        }
    }
    if (best_format)
    {
        return *best_format;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Non-?RGB8888 formats not yet supported for display"}));
}
