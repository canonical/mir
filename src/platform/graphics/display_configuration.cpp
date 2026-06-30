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

#include <mir/graphics/display_configuration.h>
#include <mir/output_type_names.h>
#include <mir/graphics/transformation.h>

extern "C"
{
#include <libdisplay-info/edid.h>
#include <libdisplay-info/info.h>
}

#include <cstdlib>
#include <format>
#include <ostream>
#include <algorithm>
#include <tuple>

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

char const* as_string(MirOrientation orientation)
{
    switch (orientation)
    {
    case mir_orientation_right:
        return "right";
    case mir_orientation_inverted:
        return "inverted";
    case mir_orientation_left:
        return "left";
    case mir_orientation_normal:
        return "normal";
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
    if (val.custom_logical_size.has_value())
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

bool mg::operator==(mg::DisplayConfigurationOutput const& val1,
                    mg::DisplayConfigurationOutput const& val2)
{
    return std::tie(val1.id, val1.card_id, val1.logical_group_id, val1.type,
                    val1.physical_size_mm, val1.preferred_mode_index, val1.connected,
                    val1.used, val1.top_left, val1.orientation, val1.current_mode_index,
                    val1.modes, val1.custom_logical_size, val1.scale, val1.form_factor) ==
           std::tie(val2.id, val2.card_id, val2.logical_group_id, val2.type,
                    val2.physical_size_mm, val2.preferred_mode_index, val2.connected,
                    val2.used, val2.top_left, val2.orientation, val2.current_mode_index,
                    val2.modes, val2.custom_logical_size, val2.scale, val2.form_factor);
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

mg::DisplayInfo::DisplayInfo(std::vector<uint8_t> const& edid)
{
    if (edid.empty())
    {
        return;
    }

    raw_edid = edid;

    if (std::unique_ptr<di_info, decltype(&di_info_destroy)> const info{
                                di_info_parse_edid(edid.data(), edid.size()),
                                &di_info_destroy})
    {
        if (std::unique_ptr<char, decltype(&free)> const di_make{di_info_get_make(info.get()), &free})
        {
            vendor.emplace(di_make.get());
        }
        if (std::unique_ptr<char, decltype(&free)> const di_model{di_info_get_model(info.get()), &free})
        {
            model.emplace(di_model.get());
        }
        if (std::unique_ptr<char, decltype(&free)> const di_serial{di_info_get_serial(info.get()), &free})
        {
            serial.emplace(di_serial.get());
        }
        if (auto const vendor_product = di_edid_get_vendor_product(di_info_get_edid(info.get())))
        {
            if (vendor_product->product)
            {
                product_code = vendor_product->product;
            }
            if (vendor_product->serial)
            {
                serial_number = vendor_product->serial;
            }
        }
    }
}

mir::geometry::Rectangle mg::DisplayConfigurationOutput::extents() const
{
    return custom_logical_size.has_value() ?
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

    if (custom_logical_size.has_value())
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
        custom_logical_size(main.custom_logical_size),
        name(main.name),
        custom_attribute{main.custom_attribute},
        display_info(main.display_info)
{
}

mir::geometry::Rectangle mg::UserDisplayConfigurationOutput::extents() const
{
    return custom_logical_size.has_value() ?
           mir::geometry::Rectangle(top_left, custom_logical_size.value()) :
           extents_of(modes, current_mode_index, orientation, top_left, scale);
}

auto std::formatter<mir::graphics::DisplayConfigurationCard>::format(
    mir::graphics::DisplayConfigurationCard const& card,
    std::format_context& ctx) const -> std::format_context::iterator
{
    return std::format_to(ctx.out(), "{{ id: {} max_simultaneous_outputs: {} }}\n", card.id, card.max_simultaneous_outputs);
}

auto std::formatter<mir::graphics::DisplayConfigurationMode>::format(
    mir::graphics::DisplayConfigurationMode const& mode,
    std::format_context& ctx) const -> std::format_context::iterator
{
    return std::format_to(ctx.out(), "{}x{}@{:.1f}", mode.size.width, mode.size.height, mode.vrefresh_hz);
}

auto std::formatter<mir::graphics::DisplayConfigurationOutput>::format(
    mir::graphics::DisplayConfigurationOutput const& output,
    std::format_context& ctx) const -> std::format_context::iterator
{
    auto out = std::format_to(
        ctx.out(),
        "{{\n\tid: {}\n\tcard_id: {}\n\tlogical_group_id: {}\n\ttype: {}\n\tmodes: [ ",
        output.id,
        output.card_id,
        output.logical_group_id,
        mir::output_type_name(static_cast<unsigned>(output.type)));

    for (size_t i = 0; i < output.modes.size(); ++i)
    {
        out = std::format_to(out, "{}", output.modes[i]);
        if (i != output.modes.size() - 1)
            out = std::format_to(out, ", ");
    }

    out = std::format_to(out, "]\n\tpreferred_mode: {}\n\tphysical_size_mm: {}x{}\n\tconnected: {}\n\tused: {}\n\ttop_left: {}\n\tcurrent_mode: {} (",
        output.preferred_mode_index,
        output.physical_size_mm.width,
        output.physical_size_mm.height,
        output.connected ? "true" : "false",
        output.used ? "true" : "false",
        output.top_left,
        output.current_mode_index);

    if (output.current_mode_index < output.modes.size())
        out = std::format_to(out, "{}", output.modes[output.current_mode_index]);
    else
        out = std::format_to(out, "none");

    out = std::format_to(out, ")\n\tscale: {}\n\tform factor: {}\n\tcustom logical size: ", output.scale, as_string(output.form_factor));
    if (output.custom_logical_size)
    {
        auto const& size = output.custom_logical_size.value();
        out = std::format_to(out, "{}x{}", size.width.as_int(), size.height.as_int());
    }
    else
    {
        out = std::format_to(out, "not set");
    }

    return std::format_to(out, "\n\torientation: {}\n}}\n", as_string(output.orientation));
}

auto std::formatter<mir::graphics::DisplayConfiguration>::format(
    mir::graphics::DisplayConfiguration const& config,
    std::format_context& ctx) const -> std::format_context::iterator
{
    auto out = ctx.out();
    config.for_each_output([&out](mir::graphics::DisplayConfigurationOutput const& output)
        {
            out = std::format_to(out, "{}\n", output);
        });
    return out;
}
