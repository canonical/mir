/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_DISPLAY_CONFIGURATION_H_

#include "mir/int_wrapper.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/point.h"
#include "mir/graphics/gamma_curves.h"
#include "mir/optional_value.h"
#include "mir_toolkit/common.h"

#include <glm/glm.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace mir
{
namespace graphics
{
namespace detail
{
struct GraphicsConfCardIdTag;
struct GraphicsConfOutputIdTag;
struct GraphicsConfLogicalGroupId;
}

typedef IntWrapper<detail::GraphicsConfCardIdTag> DisplayConfigurationCardId;
typedef IntWrapper<detail::GraphicsConfOutputIdTag> DisplayConfigurationOutputId;
typedef IntWrapper<detail::GraphicsConfLogicalGroupId> DisplayConfigurationLogicalGroupId;

/**
 * Configuration information for a display card.
 */
struct DisplayConfigurationCard
{
    DisplayConfigurationCardId id;
    size_t max_simultaneous_outputs;
};

/**
 * The type of a display output.
 */
enum class DisplayConfigurationOutputType
{
    unknown     = mir_output_type_unknown,
    vga         = mir_output_type_vga,
    dvii        = mir_output_type_dvii,
    dvid        = mir_output_type_dvid,
    dvia        = mir_output_type_dvia,
    composite   = mir_output_type_composite,
    svideo      = mir_output_type_svideo,
    lvds        = mir_output_type_lvds,
    component   = mir_output_type_component,
    ninepindin  = mir_output_type_ninepindin,
    displayport = mir_output_type_displayport,
    hdmia       = mir_output_type_hdmia,
    hdmib       = mir_output_type_hdmib,
    tv          = mir_output_type_tv,
    edp         = mir_output_type_edp,
    virt        = mir_output_type_virtual,
    dsi         = mir_output_type_dsi,
    dpi         = mir_output_type_dpi,
};

/**
 * Configuration information for a display output mode.
 */
struct DisplayConfigurationMode
{
    geometry::Size size;
    double vrefresh_hz;
};

/**
 * Hardware information for a display output.
 */
struct DisplayInfo
{
    DisplayInfo() = default;
    explicit DisplayInfo(std::vector<uint8_t> const& edid);
    DisplayInfo& operator=(std::vector<uint8_t> const& edid);

    std::vector<uint8_t> raw_edid;
    std::optional<std::string> vendor;
    std::optional<std::string> model;
    std::optional<std::string> serial;
    std::optional<uint16_t> product_code;
};

/**
 * Configuration information for a display output.
 */
struct DisplayConfigurationOutput
{
    /** The output's id. */
    DisplayConfigurationOutputId id;
    /** The id of the card the output is connected to. */
    DisplayConfigurationCardId card_id;
    /** The id of the logical output group (aka display wall) this output belongs to, or 0 for none. */
    DisplayConfigurationLogicalGroupId logical_group_id;
    /** The type of the output. */
    DisplayConfigurationOutputType type;
    /** The pixel formats supported by the output */
    std::vector<MirPixelFormat> pixel_formats;
    /** The modes supported by the output. */
    std::vector<DisplayConfigurationMode> modes;
    /** The index in the 'modes' vector of the preferred output mode. */
    uint32_t preferred_mode_index;
    /** The physical size of the output. */
    geometry::Size physical_size_mm;
    /** Whether the output is connected. */
    bool connected;
    /** Whether the output is used in the configuration. */
    bool used;
    /** The top left point of this output in the virtual coordinate space. */
    geometry::Point top_left;
    /** The index in the 'modes' vector of the current output mode. */
    uint32_t current_mode_index;
    /** The current output pixel format. A matching entry should be found in the 'pixel_formats' vector*/
    MirPixelFormat current_format;
    /** Current power mode **/
    MirPowerMode power_mode;
    MirOrientation orientation;

    /** Requested scale factor for this output, for HiDPI support */
    float scale{1.0f};
    /** Form factor of this output; phone display, tablet, monitor, TV, projector... */
    MirFormFactor form_factor;

    /** Subpixel arrangement of this output */
    MirSubpixelArrangement subpixel_arrangement;

    /** The current gamma for the display */
    GammaCurves gamma;
    MirOutputGammaSupported gamma_supported;

    mir::optional_value<geometry::Size> custom_logical_size;

    /** The output's displayable name. */
    // TODO: Better output names that are consistant between sessions
    std::string name = "OUT-" + std::to_string(id.as_value());

    /// Custom attributes (typically set via the .display configuration file
    std::map<std::string, std::optional<std::string>> custom_attribute = {};

    /// Display information for this output, if available.
    DisplayInfo display_info = {};

    /** The logical rectangle occupied by the output, based on its position,
        current mode and orientation (rotation) */
    geometry::Rectangle extents() const;
    bool valid() const;

    /**
     * The additional transformation (if any) that this output must undergo
     * to appear correctly on screen, including rotation (orientation),
     * inversion and skew.
     *   Note that scaling and translation are not part of this transformation
     * matrix because those are already built in to the extents() rectangle
     * for the renderer to use in compositing.
     */
    glm::mat2 transformation() const;
};

/**
 * Mirror of a DisplayConfigurationOutput, with some fields limited to
 * being read-only, preventing users from changing things they shouldn't.
 */
struct UserDisplayConfigurationOutput
{
    DisplayConfigurationOutputId const& id;
    DisplayConfigurationCardId const& card_id;
    DisplayConfigurationLogicalGroupId& logical_group_id;
    DisplayConfigurationOutputType const& type;
    std::vector<MirPixelFormat> const& pixel_formats;
    std::vector<DisplayConfigurationMode> const& modes;
    uint32_t const& preferred_mode_index;
    geometry::Size const& physical_size_mm;
    bool const& connected;
    bool& used;
    geometry::Point& top_left;
    uint32_t& current_mode_index;
    MirPixelFormat& current_format;
    MirPowerMode& power_mode;
    MirOrientation& orientation;
    float& scale;
    MirFormFactor& form_factor;
    MirSubpixelArrangement& subpixel_arrangement;
    GammaCurves& gamma;
    MirOutputGammaSupported const& gamma_supported;
    mir::optional_value<geometry::Size>& custom_logical_size;
    std::string const& name;
    /// Custom attributes (typically set by the .display configuration file
    std::map<std::string, std::optional<std::string>>& custom_attribute;
    DisplayInfo const& display_info;

    UserDisplayConfigurationOutput(DisplayConfigurationOutput& main);
    geometry::Rectangle extents() const;
};

std::ostream& operator<<(std::ostream& out, DisplayConfigurationCard const& val);
bool operator==(DisplayConfigurationCard const& val1, DisplayConfigurationCard const& val2);
bool operator!=(DisplayConfigurationCard const& val1, DisplayConfigurationCard const& val2);

std::ostream& operator<<(std::ostream& out, DisplayConfigurationMode const& val);
bool operator==(DisplayConfigurationMode const& val1, DisplayConfigurationMode const& val2);
bool operator!=(DisplayConfigurationMode const& val1, DisplayConfigurationMode const& val2);

std::ostream& operator<<(std::ostream& out, DisplayConfigurationOutput const& val);
bool operator==(DisplayConfigurationOutput const& val1, DisplayConfigurationOutput const& val2);
bool operator!=(DisplayConfigurationOutput const& val1, DisplayConfigurationOutput const& val2);

/**
 * Interface to a configuration of display cards and outputs.
 */
class DisplayConfiguration
{
public:
    virtual ~DisplayConfiguration() = default;

    /** Executes a function object for each output in the configuration. */
    virtual void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const = 0;
    virtual void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) = 0;

    virtual std::unique_ptr<DisplayConfiguration> clone() const = 0;

    virtual bool valid() const;

protected:
    DisplayConfiguration() = default;
    DisplayConfiguration(DisplayConfiguration const& c) = delete;
    DisplayConfiguration& operator=(DisplayConfiguration const& c) = delete;
};

bool operator==(DisplayConfiguration const& lhs, DisplayConfiguration const& rhs);
bool operator!=(DisplayConfiguration const& lhs, DisplayConfiguration const& rhs);

std::ostream& operator<<(std::ostream& out, DisplayConfiguration const& val);

}
}

#endif /* MIR_GRAPHICS_DISPLAY_CONFIGURATION_H_ */
