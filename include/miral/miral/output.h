/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_OUTPUT_H
#define MIRAL_OUTPUT_H

#include <mir_toolkit/common.h>

#include <mir/geometry/rectangle.h>

#include <map>
#include <memory>
#include <optional>

namespace mir { namespace graphics { struct DisplayConfigurationOutput; } }

namespace miral
{
using namespace mir::geometry;

/// Describes information about a display output.
///
/// Compositor authors may be notified about outputs via
/// #miral::WindowManagementPolicy::advise_output_create.
///
/// Authors may also use the #miral::DisplayConfiguration to set certain
/// properties on the output via a configuration file.
///
/// \sa miral::WindowManagementPolicy::advise_output_create - the output
/// \sa miral::DisplayConfiguration - outputs can get their configuration from this class
class Output
{
public:
    /// Describes the physical size of the output in millimeters.
    struct PhysicalSizeMM { int width; int height; };

    /// The type of the output.
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
        edp,
        virt,
        dsi,
        dpi
    };

    /// Construct an output from its configuration.
    ///
    /// \param output the configuration for this output
    explicit Output(const mir::graphics::DisplayConfigurationOutput &output);
    Output(Output const&);
    Output& operator=(Output const&);
    ~Output();

    /// The type of the output.
    ///
    /// \returns the type
    auto type() const -> Type;

    /// The physical size of the output.
    ///
    /// \returns the physical size
    auto physical_size_mm() const -> PhysicalSizeMM;

    /// Whether the output is connected.
    ///
    /// \returns `true` if connected, otherwise `false`
    auto connected() const -> bool;

    /// Whether the output is used in the configuration.
    ///
    /// \returns `true` if used, otherwise `false`
    auto used() const -> bool;

    /// The current output pixel format.
    ///
    /// \returns the pixel format
    auto pixel_format() const -> MirPixelFormat;
    
    /// The refresh rate in Hz.
    ///
    /// \returns the refresh rate
    auto refresh_rate() const -> double;

    /// The current power mode.
    ///
    /// \returns the power mode
    auto power_mode() const -> MirPowerMode;

    /// The orientation of the output.
    ///
    /// \returns the orientation
    auto orientation() const -> MirOrientation;

    /// Requested scale factor for this output, for HiDPI support.
    ///
    /// \returns the scale
    auto scale() const -> float;

    /// Form factor of this output, e.g. a phone display, tablet, monitor, TV, projector, and more.
    ///
    /// \returns the form factor
    auto form_factor() const -> MirFormFactor;

    /// The logical rectangle occupied by the output, based on its position,
    /// current mode and orientation (rotation).
    ///
    /// \returns the extents
    auto extents() const -> Rectangle;

    /// Mir's internal output ID.
    ///
    /// This is useful for matching against a #miral::WindowInfo::output_id.
    ///
    /// \returns the id
    auto id() const -> int;

    /// The output name.
    ///
    /// This matches the name supplied to Wayland clients through `wl_output::name`.
    ///
    /// \returns the name
    auto name() const -> std::string;

    /// Returns the custom attribute on this output by \p key.
    ///
    /// Custom attributes may be registered on outputs via:
    /// #miral::DisplayConfiguration::add_output_attribute.
    ///
    /// \param key the key
    /// \returns the value at the \p key, if any
    auto attribute(std::string const& key) const -> std::optional<std::string>;

    /// Returns the entire attribute map on this output.
    ///
    /// \returns the raw attributes of the output
    auto attributes_map() const -> std::map<std::string const, std::optional<std::string>>;

    /// Check if this output is configured properly.
    ///
    /// \returns `true` if the configuration is valid, otherwise `false`
    auto valid() const -> bool;

    /// Check if the provided output is the same as this one.
    ///
    /// \param other output to compare with
    /// \returns `true` if they are the same, otherwise `false`
    auto is_same_output(Output const& other) const -> bool;

    /// A positive number if this output is part of a logical output group (e.g. a display wall).
    ///
    /// A single display area will stretch across all outputs in a group.
    /// This value is 0 if this output is not part of a logical group.
    ///
    /// Users may set the group ID of the output through the #miral::DisplayConfiguration.
    ///
    /// \returns the group id
    auto logical_group_id() const -> int;

private:
    std::shared_ptr<mir::graphics::DisplayConfigurationOutput> self;
};

bool operator==(Output::PhysicalSizeMM const& lhs, Output::PhysicalSizeMM const& rhs);
inline bool operator!=(Output::PhysicalSizeMM const& lhs, Output::PhysicalSizeMM const& rhs)
{ return !(lhs == rhs); }

/// Checks if the provided outputs have the same display area.
///
/// \param lhs first output
/// \param rhs second output
/// \returns `true` if their display area is the same, otherwise false
auto equivalent_display_area(Output const& lhs, Output const& rhs) -> bool;
}

#endif //MIRAL_OUTPUT_H
