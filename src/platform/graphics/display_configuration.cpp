/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/display_configuration.h"

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

char const* output_type_to_string(mg::DisplayConfigurationOutputType type)
{
    static char const* type_names[] =
    {
        "unknown",
        "vga",
        "dvii",
        "dvid",
        "dvia",
        "composite",
        "lvds",
        "component",
        "9pindin",
        "displayport",
        "hdmia",
        "hdmib",
        "tv",
        "edp"
    };

    auto index = static_cast<ssize_t>(type);
    static auto const size = std::distance(std::begin(type_names), std::end(type_names));
    if (index >= size || index < 0)
        return "invalid";

    return type_names[index];
}

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
    out << "{\n\tid: " << val.id << "\n\tcard_id: " << val.card_id
        << "\n\ttype: " << output_type_to_string(val.type)
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
    out << "}" << std::endl;

    return out;
}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfiguration const& val)
{
    val.for_each_card([&out](auto card) { out << card << std::endl; });
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
               (val1.type == val2.type) &&
               (val1.physical_size_mm == val2.physical_size_mm) &&
               (val1.preferred_mode_index == val2.preferred_mode_index) &&
               (val1.connected == val2.connected) &&
               (val1.used == val2.used) &&
               (val1.top_left == val2.top_left) &&
               (val1.orientation == val2.orientation) &&
               (val1.current_mode_index == val2.current_mode_index) &&
               (val1.modes.size() == val2.modes.size()) &&
               (val1.scale == val2.scale) &&
               (val1.form_factor == val2.form_factor)};

    if (equal)
    {
        for (size_t i = 0; i < val1.modes.size(); i++)
        {
            equal = equal && (val1.modes[i] == val2.modes[i]);
            if (!equal) break;
        }
    }

    return equal;
}

bool mg::operator!=(mg::DisplayConfigurationOutput const& val1,
                    mg::DisplayConfigurationOutput const& val2)
{
    return !(val1 == val2);
}

namespace
{
mir::geometry::Rectangle extents_of(
    std::vector<mg::DisplayConfigurationMode> const& modes,
    size_t current_mode_index,
    MirOrientation orientation,
    mir::geometry::Point top_left)
{
    if (current_mode_index > modes.size())
        return mir::geometry::Rectangle();

    auto const& size = modes[current_mode_index].size;

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
    return extents_of(modes, current_mode_index, orientation, top_left);
}

bool mg::DisplayConfigurationOutput::valid() const
{
    if (!connected)
        return !used;

    auto const& f = std::find(pixel_formats.begin(), pixel_formats.end(),
                              current_format);
    if (f == pixel_formats.end())
        return false;

    auto nmodes = modes.size();
    if (current_mode_index >= nmodes)
        return false;

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
    DisplayConfigurationOutput& master) :
        id(master.id),
        card_id(master.card_id),
        type(master.type),
        pixel_formats(master.pixel_formats),
        modes(master.modes),
        preferred_mode_index(master.preferred_mode_index),
        physical_size_mm(master.physical_size_mm),
        connected(master.connected),
        used(master.used),
        top_left(master.top_left),
        current_mode_index(master.current_mode_index),
        current_format(master.current_format),
        power_mode(master.power_mode),
        orientation(master.orientation),
        scale(master.scale),
        form_factor(master.form_factor)
{
}

mir::geometry::Rectangle mg::UserDisplayConfigurationOutput::extents() const
{
    return extents_of(modes, current_mode_index, orientation, top_left);
}


