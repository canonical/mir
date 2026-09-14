/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIROIL_DISPLAY_CONFIGURATION_STORAGE_H
#define MIROIL_DISPLAY_CONFIGURATION_STORAGE_H

#include <miroil/display_id.h>

#include <mir/geometry/rectangle.h>
#include <optional>
#include <mir_toolkit/common.h>

#include <sys/types.h>

namespace miroil
{

struct DisplayConfigurationOptions
{
    std::optional<bool> used;
    std::optional<uint> clone_output_index;
    struct DisplayMode
    {
        mir::geometry::Size size;
        double refresh_rate{-1};
    };
    std::optional<DisplayMode> mode;
    std::optional<MirOrientation> orientation;
    std::optional<MirFormFactor> form_factor;
    std::optional<float> scale;
};

class DisplayConfigurationStorage
{
public:
    virtual ~DisplayConfigurationStorage() = default;

    virtual void save(DisplayId const&, DisplayConfigurationOptions const&) = 0;
    virtual bool load(DisplayId const&, DisplayConfigurationOptions&) const = 0;
};

} // namespace miroil

#endif // MIROIL_DISPLAY_CONFIGURATION_STORAGE_H
