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

#include "mir/graphics/display_configuration.h"
#include "mir/output_type_names.h"
#include "mir/graphics/transformation.h"

#include <ostream>
#include <algorithm>

namespace mg = mir::graphics;

namespace
{

class StreamPropertiesRecovery
{
public:
    StreamPropertiesRecovery(std::ostream& stream)
        : stream(stream),
          flags{stream.flags()},
          precision{stream.precision()}
    {
    }

    ~StreamPropertiesRecovery()
    {
        stream.precision(precision);
        stream.flags(flags);
    }

private:
    std::ostream& stream;
    std::ios_base::fmtflags const flags;
    std::streamsize const precision;
};

}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfigurationCard const& val)
{
    return out << "{ id: " << val.id
               << " max_simultaneous_outputs: " << val.max_simultaneous_outputs << " }"
               << std::endl;
}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfigurationMode const& val)
{
    StreamPropertiesRecovery const stream_properties_recovery{out};

    out.precision(1);
    out.setf(std::ios_base::fixed);

    return out << val.size.width << "x" << val.size.height << "@" << val.vrefresh_hz;
}

namespace
{
char const* as_string(MirFormFactor form_factor)
{
    switch (form_factor)
    {
    case mir_form_factor_monitor:
        return "monitor";
    case mir_form_factor_projector:
        return "projector";
    case mir_form_factor_tv:
        return "TV";
    case mir_form_factor_phone:
        return "phone";
    case mir_form_factor_tablet:
        return "tablet";
    default:
        return "UNKNOWN";
    }
}
}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfigurationOutput const& val)
{
    out << "{\n\tid: " << val.id << "\n\tcard_id: " << val.card_id << "\n\tlogical_group_id: " << val.logical_group_id
        << "\n\ttype: " << mir::output_type_name(static_cast<unsigned>(val.type))
        << "\n\tmodes: [ ";

    for (size_t i = 0; i < val.modes.size(); ++i)
    {
        out << val.modes[i];
        if (i != val.modes.size() - 1)
            out << ", ";
    }

    out << "]" << std::endl;
    out << "\tpreferred_mode: " << val.preferred_mode_index << std::endl;
    out << "\tphysical_size_mm: " << val.physical_size_mm.width << "x" << val.physical_size_mm.height << std::endl;
    out << "\tconnected: " << (val.connected ? "true" : "false") << std::endl;
    out << "\tused: " << (val.used ? "true" : "false") << std::endl;
    out << "\ttop_left: " << val.top_left << std::endl;
    out << "\tcurrent_mode: " << val.current_mode_index << " (";
    if (val.current_mode_index < val.modes.size())
        out << val.modes[val.current_mode_index];
    else
        out << "none";
    out << ")" << std::endl;

    out << "\tscale: " << val.scale << std::endl;
    out << "\tform factor: " << as_string(val.form_factor) << std::endl;

    out << "\tcustom logical size: ";
    if (val.custom_logical_size.is_set())
    {
        auto const& size = val.custom_logical_size.value();
        out << size.width.as_int() << "x" << size.height.as_int();
    }
    else
    {
        out << "not set";
    }
    out << std::endl;

    out << "\torientation: " << val.orientation << '\n';
    out << "}" << std::endl;

    return out;
}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfiguration const& val)
{
    val.for_each_output([&out](DisplayConfigurationOutput const& output) { out << output << std::endl; });

    return out;
}

bool mg::operator==(mg::DisplayConfigurationCard const& val1,
                    mg::DisplayConfigurationCard const& val2)
{
    return (val1.id == val2.id) &&
           (val1.max_simultaneous_outputs == val2.max_simultaneous_outputs);
}

bool mg::operator!=(mg::DisplayConfigurationCard const& val1,
                    mg::DisplayConfigurationCard const& val2)
{
    return !(val1 == val2);
}

bool mg::operator==(mg::DisplayConfigurationMode const& val1,
                    mg::DisplayConfigurationMode const& val2)
{
    return (val1.size == val2.size) &&
           (val1.vrefresh_hz == val2.vrefresh_hz);
}

bool mg::operator!=(mg::DisplayConfigurationMode const& val1,
                    mg::DisplayConfigurationMode const& val2)
{
    return !(val1 == val2);
}

bool mg::operator==(mg::DisplayConfigurationOutput const& val1,
                    mg::DisplayConfigurationOutput const& val2)
{
    bool equal{(val1.id == val2.id) &&
               (val1.card_id == val2.card_id) &&
               (val1.logical_group_id == val2.logical_group_id) &&
               (val1.type == val2.type) &&
               (val1.physical_size_mm == val2.physical_size_mm) &&
               (val1.preferred_mode_index == val2.preferred_mode_index) &&
               (val1.connected == val2.connected) &&
               (val1.used == val2.used) &&
               (val1.top_left == val2.top_left) &&
               (val1.orientation == val2.orientation) &&
               (val1.current_mode_index == val2.current_mode_index) &&
               (val1.modes.size() == val2.modes.size()) &&
               (val1.custom_logical_size == val2.custom_logical_size) &&
               (val1.scale == val2.scale) &&
               (val1.form_factor == val2.form_factor)};

    for (auto i = begin(val1.modes), j = begin(val2.modes); i != end(val1.modes) && equal; ++i, ++j)
    {
        equal = *i == *j;
    }

    return equal;
}

bool mg::operator!=(mg::DisplayConfigurationOutput const& val1,
                    mg::DisplayConfigurationOutput const& val2)
{
    return !(val1 == val2);
}

bool mg::operator==(DisplayConfiguration const& lhs, DisplayConfiguration const& rhs)
{
    std::vector<DisplayConfigurationCard> lhs_cards;
    std::vector<DisplayConfigurationOutput> lhs_outputs;

    lhs.for_each_output([&lhs_outputs](DisplayConfigurationOutput const& output) { lhs_outputs.emplace_back(output); });

    std::vector<DisplayConfigurationCard> rhs_cards;
    std::vector<DisplayConfigurationOutput> rhs_outputs;

    rhs.for_each_output([&rhs_outputs](DisplayConfigurationOutput const& output) { rhs_outputs.emplace_back(output); });

    return lhs_cards == rhs_cards && lhs_outputs == rhs_outputs;
}

bool mg::operator!=(DisplayConfiguration const& lhs, DisplayConfiguration const& rhs)
{
    return !(lhs == rhs);
}

namespace
{
mir::geometry::Rectangle extents_of(
    std::vector<mg::DisplayConfigurationMode> const& modes,
    size_t current_mode_index,
    MirOrientation orientation,
    mir::geometry::Point top_left,
    float scale)
{
    if (current_mode_index >= modes.size())
        return mir::geometry::Rectangle();

    mir::geometry::Size const size{
        roundf(modes[current_mode_index].size.width.as_int() / scale),
        roundf(modes[current_mode_index].size.height.as_int() / scale)};

    if (orientation == mir_orientation_normal ||
        orientation == mir_orientation_inverted)
    {
        return {top_left, size};
    }
    else
    {
        return {top_left, {size.height.as_int(), size.width.as_int()}};
    }
}

}

mir::geometry::Rectangle mg::DisplayConfigurationOutput::extents() const
{
    return custom_logical_size.is_set() ?
           mir::geometry::Rectangle(top_left, custom_logical_size.value()) :
           extents_of(modes, current_mode_index, orientation, top_left, scale);
}

glm::mat2 mg::DisplayConfigurationOutput::transformation() const
{
    return mg::transformation(orientation);
}

bool mg::DisplayConfigurationOutput::valid() const
{
    if (!connected)
        return !used;

    auto const& f = std::find(pixel_formats.begin(), pixel_formats.end(),
                              current_format);
    if (f == pixel_formats.end())
        return false;

    if (used && current_mode_index >= modes.size())
        return false;

    if (custom_logical_size.is_set())
    {
        auto const& logical_size = custom_logical_size.value();
        if (!logical_size.width.as_int() || !logical_size.height.as_int())
            return false;
    }

    return true;
}

bool mg::DisplayConfiguration::valid() const
{
    bool all_valid = true;

    for_each_output([&all_valid](DisplayConfigurationOutput const& out)
        {
            if (!out.valid())
                all_valid = false;
        });

    return all_valid;
}

mg::UserDisplayConfigurationOutput::UserDisplayConfigurationOutput(
    DisplayConfigurationOutput& main) :
        id(main.id),
        card_id(main.card_id),
        logical_group_id{main.logical_group_id},
        type(main.type),
        pixel_formats(main.pixel_formats),
        modes(main.modes),
        preferred_mode_index(main.preferred_mode_index),
        physical_size_mm(main.physical_size_mm),
        connected(main.connected),
        used(main.used),
        top_left(main.top_left),
        current_mode_index(main.current_mode_index),
        current_format(main.current_format),
        power_mode(main.power_mode),
        orientation(main.orientation),
        scale(main.scale),
        form_factor(main.form_factor),
        subpixel_arrangement(main.subpixel_arrangement),
        gamma(main.gamma),
        gamma_supported(main.gamma_supported),
        edid(main.edid),
        custom_logical_size(main.custom_logical_size),
        name(main.name),
        custom_attribute{main.custom_attribute}
{
}

mir::geometry::Rectangle mg::UserDisplayConfigurationOutput::extents() const
{
    return custom_logical_size.is_set() ?
           mir::geometry::Rectangle(top_left, custom_logical_size.value()) :
           extents_of(modes, current_mode_index, orientation, top_left, scale);
}


