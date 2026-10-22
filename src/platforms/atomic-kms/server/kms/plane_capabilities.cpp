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

#include "plane_capabilities.h"

#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/graphics/kms/kms_connector.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>
#include <drm_mode.h>
#include <xf86drmMode.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>

#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mgk = mir::graphics::kms;

namespace
{
using BlobUPtr = std::unique_ptr<drmModePropertyBlobRes, void (*)(drmModePropertyBlobPtr)>;

auto is_range(drmModePropertyRes const& prop) -> bool
{
    return prop.count_values >= 2 &&
        (prop.flags & DRM_MODE_PROP_RANGE) != 0 &&
        prop.values[0] <= prop.values[1];
}

auto enum_value(drmModePropertyRes const& prop, std::string_view name) -> std::optional<mga::PropertyValue>
{
    std::span const enums{prop.enums, static_cast<size_t>(prop.count_enums)};
    auto const it = std::ranges::find(
        enums, name, [](drm_mode_property_enum const& entry) { return std::string_view{entry.name}; });
    return it == enums.end() ? std::nullopt : std::optional<mga::PropertyValue>{it->value};
}

template<typename T>
auto read_at(std::span<std::byte const> data, size_t offset) -> T
{
    T value{};
    std::memcpy(&value, data.data() + offset, sizeof value);
    return value;
}

template<typename T>
auto read_array(std::span<std::byte const> data, uint32_t offset, uint32_t count) -> std::optional<std::vector<T>>
{
    if (uint64_t{offset} + uint64_t{count} * sizeof(T) > data.size())
        return std::nullopt;

    std::vector<T> values(count);
    if (count != 0)
        std::memcpy(values.data(), data.data() + offset, count * sizeof(T));
    return values;
}

/// Returns false if \a entry marks a format past the end of \a formats
auto append_pairs_for_modifier(
    drm_format_modifier const& entry,
    std::span<uint32_t const> formats,
    std::vector<mga::FormatModifier>& pairs) -> bool
{
    for (uint64_t bit = 0; bit < 64; ++bit)
    {
        if ((entry.formats & (uint64_t{1} << bit)) == 0)
            continue;

        auto const index = uint64_t{entry.offset} + bit;
        if (index >= formats.size())
            return false;
        pairs.push_back({formats[index], entry.modifier});
    }
    return true;
}

auto parse_in_formats(std::span<std::byte const> data) -> std::optional<std::vector<mga::FormatModifier>>
{
    if (data.size() < sizeof(drm_format_modifier_blob))
        return std::nullopt;

    auto const header = read_at<drm_format_modifier_blob>(data, 0);
    if (header.version != FORMAT_BLOB_CURRENT ||
        header.formats_offset < sizeof header ||
        header.modifiers_offset < sizeof header)
        return std::nullopt;

    auto const formats = read_array<uint32_t>(data, header.formats_offset, header.count_formats);
    auto const modifiers = read_array<drm_format_modifier>(data, header.modifiers_offset, header.count_modifiers);
    if (!formats || !modifiers)
        return std::nullopt;

    std::vector<mga::FormatModifier> pairs;
    for (auto const& entry : *modifiers)
    {
        if (!append_pairs_for_modifier(entry, *formats, pairs))
            return std::nullopt;
    }

    return pairs.empty() ? std::nullopt : std::optional{std::move(pairs)};
}

auto decode_formats(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<std::vector<mga::FormatModifier>>
{
    auto const blob_id = props.has_property("IN_FORMATS") ? props["IN_FORMATS"] : 0;
    if (blob_id == 0)
        return std::nullopt;

    errno = 0;
    BlobUPtr const blob{drmModeGetPropertyBlob(drm_fd, static_cast<uint32_t>(blob_id)), &drmModeFreePropertyBlob};
    if (!blob)
    {
        // drmModeGetPropertyBlob either sets errno, or has failed in malloc()
        BOOST_THROW_EXCEPTION((std::system_error{
            errno ? errno : ENOMEM,
            std::system_category(),
            "Failed to read IN_FORMATS blob of plane " + std::to_string(props.parent_id())}));
    }

    return parse_in_formats({static_cast<std::byte const*>(blob->data), blob->length});
}

auto decode_plane(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<mga::PlaneCapability>
{
    auto const zpos_prop = props.metadata("zpos");
    if (!zpos_prop || !is_range(*zpos_prop))
        return std::nullopt;
    auto const zpos = mga::ZPos{
        .min = static_cast<int64_t>(zpos_prop->values[0]),
        .max = static_cast<int64_t>(zpos_prop->values[1]),
        .current = static_cast<int64_t>(props["zpos"]),
        .immutable = (zpos_prop->flags & DRM_MODE_PROP_IMMUTABLE) != 0,
    };

    auto formats = decode_formats(drm_fd, props);
    if (!formats)
        return std::nullopt;

    auto const alpha = props.metadata("alpha");
    if (alpha && !is_range(*alpha))
        return std::nullopt;

    auto const blend_prop = props.metadata("pixel blend mode");

    auto const encoding_prop = props.metadata("COLOR_ENCODING");
    auto const range_prop = props.metadata("COLOR_RANGE");
    auto const encoding = encoding_prop ? enum_value(*encoding_prop, "ITU-R BT.709 YCbCr") : std::nullopt;
    auto const range = range_prop ? enum_value(*range_prop, "YCbCr limited range") : std::nullopt;

    return mga::PlaneCapability{
        .id = props.parent_id(),
        .zpos = zpos,
        .formats = std::move(*formats),
        .alpha_max = alpha ? std::optional{alpha->values[1]} : std::nullopt,
        .blend_none = blend_prop ? enum_value(*blend_prop, "None") : std::nullopt,
        .fixed_opaque = !alpha && !blend_prop,
        .bt709_limited = encoding && range ? std::optional{mga::Bt709Limited{*encoding, *range}} : std::nullopt,
    };
}

}

auto mga::discover_crtc_plane_capabilities(int drm_fd, uint32_t crtc_id) -> CrtcPlaneCapabilities
{
    mgk::DRMModeResources const resources{drm_fd};
    auto const crtc_index = mgk::find_crtc_index(resources, crtc_id);
    if (!crtc_index)
        BOOST_THROW_EXCEPTION(std::runtime_error{"CRTC " + std::to_string(crtc_id) + " not found"});
    auto const crtc_mask = uint32_t{1} << *crtc_index;

    mgk::PlaneResources plane_resources{drm_fd};
    auto const primary_plane = mgk::find_primary_plane_for_crtc(drm_fd, plane_resources, *crtc_index);
    auto primary = primary_plane ? decode_plane(drm_fd, mgk::ObjectProperties{drm_fd, *primary_plane}) : std::nullopt;
    if (!primary)
        BOOST_THROW_EXCEPTION(std::runtime_error{"No usable primary plane for CRTC " + std::to_string(crtc_id)});

    auto const primary_key = std::pair{primary->zpos.current, primary->id};
    std::string fixed_opaque_ids = primary->fixed_opaque ? std::to_string(primary->id) : "";
    auto const overlay_key = [](PlaneCapability const& overlay)
        { return std::pair{overlay.zpos.current, overlay.id}; };

    std::vector<PlaneCapability> overlays;
    for (auto const& plane : plane_resources.planes())
    {
        if ((plane->possible_crtcs & crtc_mask) == 0)
            continue;

        mgk::ObjectProperties const props{drm_fd, plane};
        auto const type = props.has_property("type") ? std::optional{props["type"]} : std::nullopt;
        if (!type)
        {
            mir::log_warning("Ignoring plane %u with no type", props.parent_id());
            continue;
        }

        switch (*type)
        {
        case DRM_PLANE_TYPE_OVERLAY:
            if (auto overlay = decode_plane(drm_fd, props))
            {
                overlay->can_be_above_primary = std::pair{overlay->zpos.max, overlay->id} > primary_key;
                overlay->can_be_below_primary = std::pair{overlay->zpos.min, overlay->id} < primary_key;
                if (overlay->fixed_opaque)
                    fixed_opaque_ids += (fixed_opaque_ids.empty() ? "" : ", ") + std::to_string(overlay->id);
                auto const pos = std::ranges::lower_bound(overlays, overlay_key(*overlay), {}, overlay_key);
                overlays.insert(pos, std::move(*overlay));
            }
            break;
        case DRM_PLANE_TYPE_PRIMARY:
        case DRM_PLANE_TYPE_CURSOR:
            break;
        default:
            mir::log_warning("Ignoring plane %u with unknown type", props.parent_id());
            break;
        }
    }
    if (!fixed_opaque_ids.empty())
    {
        mir::log_info(
            "Planes treated as fixed-opaque on CRTC %u (no alpha or blend control): %s",
            crtc_id,
            fixed_opaque_ids.c_str());
    }

    return CrtcPlaneCapabilities{
        .crtc_id = crtc_id,
        .primary = std::move(*primary),
        .overlays = std::move(overlays),
    };
}
