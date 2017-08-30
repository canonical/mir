/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_OUTPUT_H
#define MIRAL_OUTPUT_H

#include <mir_toolkit/common.h>

#include <mir/geometry/rectangle.h>
#include <mir/int_wrapper.h>

#include <memory>

namespace mir { namespace graphics { struct DisplayConfigurationOutput; } }

namespace miral
{
using namespace mir::geometry;

class Output
{
public:

    struct PhysicalSizeMM { int width; int height; };

    enum class Type
    {
        unknown,
        vga,
        dvii,
        dvid,
        dvia,
        composite,
        svideo,
        lvds,
        component,
        ninepindin,
        displayport,
        hdmia,
        hdmib,
        tv,
        edp
    };

    explicit Output(const mir::graphics::DisplayConfigurationOutput &output);
    Output(Output const&);
    Output& operator=(Output const&);
    ~Output();

    /// The type of the output.
    auto type() const -> Type;

    /// The physical size of the output.
    auto physical_size_mm() const -> PhysicalSizeMM;

    /// Whether the output is connected.
    auto connected() const -> bool;

    /// Whether the output is used in the configuration.
    auto used() const -> bool;

    /// The current output pixel format.
    auto pixel_format() const -> MirPixelFormat;
    
    /// refresh_rate in Hz
    auto refresh_rate() const -> double;

    /// Current power mode
    auto power_mode() const -> MirPowerMode;

    auto orientation() const -> MirOrientation;

    /// Requested scale factor for this output, for HiDPI support
    auto scale() const -> float;

    /// Form factor of this output; phone display, tablet, monitor, TV, projector...
    auto form_factor() const -> MirFormFactor;

    /// The logical rectangle occupied by the output, based on its position,
    /// current mode and orientation (rotation)
    auto extents() const -> Rectangle;

    auto valid() const -> bool;

    auto is_same_output(Output const& other) const -> bool;

private:
    std::shared_ptr<mir::graphics::DisplayConfigurationOutput> self;
};

bool operator==(Output::PhysicalSizeMM const& lhs, Output::PhysicalSizeMM const& rhs);
inline bool operator!=(Output::PhysicalSizeMM const& lhs, Output::PhysicalSizeMM const& rhs)
{ return !(lhs == rhs); }

auto equivalent_display_area(Output const& lhs, Output const& rhs) -> bool;
}

#endif //MIRAL_OUTPUT_H
