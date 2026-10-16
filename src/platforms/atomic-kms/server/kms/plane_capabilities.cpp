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
#include <mir/log.h>

#include <boost/throw_exception.hpp>
#include <drm_mode.h>
#include <xf86drmMode.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mgk = mir::graphics::kms;

namespace
{
using BlobUPtr = std::unique_ptr<drmModePropertyBlobRes, void (*)(drmModePropertyBlobPtr)>;

auto find_property(int drm_fd, mgk::ObjectProperties const& props, char const* name) -> mgk::DRMModePropertyUPtr
{
    if (!props.has_property(name))
        return {nullptr, &drmModeFreeProperty};
    return {drmModeGetProperty(drm_fd, props.id_for(name)), &drmModeFreeProperty};
}

auto is_range(drmModePropertyRes const& prop) -> bool
{
    return prop.count_values >= 2 &&
        ((prop.flags & DRM_MODE_PROP_RANGE) != 0 || drmModeGetPropertyType(&prop) == DRM_MODE_PROP_SIGNED_RANGE);
}

auto enum_value(mgk::DRMModePropertyUPtr const& prop, std::string_view name) -> std::optional<mga::PropertyValue>
{
    if (!prop)
        return std::nullopt;

    std::span const enums{prop->enums, static_cast<size_t>(prop->count_enums)};
    auto const it = std::ranges::find(
        enums, name, [](drm_mode_property_enum const& entry) { return std::string_view{entry.name}; });
    if (it == enums.end())
        return std::nullopt;
    return it->value;
}

auto crtc_index_of(int drm_fd, uint32_t crtc_id) -> uint32_t
{
    mgk::DRMModeResources const resources{drm_fd};
    uint32_t index{0};
    for (auto const& crtc : resources.crtcs())
    {
        if (crtc && crtc->crtc_id == crtc_id)
            return index;
        ++index;
    }
    BOOST_THROW_EXCEPTION(std::runtime_error{"CRTC " + std::to_string(crtc_id) + " not found"});
}

auto decode_zpos(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<mga::ZPos>
{
    auto const prop = find_property(drm_fd, props, "zpos");
    if (!prop || !is_range(*prop))
        return std::nullopt;

    return mga::ZPos{
        .min = static_cast<int64_t>(prop->values[0]),
        .max = static_cast<int64_t>(prop->values[1]),
        .current = static_cast<int64_t>(props["zpos"]),
        .immutable = (prop->flags & DRM_MODE_PROP_IMMUTABLE) != 0,
    };
}

auto decode_alpha_max(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<mga::PropertyValue>
{
    auto const prop = find_property(drm_fd, props, "alpha");
    if (!prop || !is_range(*prop))
        return std::nullopt;
    return prop->values[1];
}

auto decode_bt709_limited(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<mga::Bt709Limited>
{
    auto const encoding = enum_value(find_property(drm_fd, props, "COLOR_ENCODING"), "ITU-R BT.709 YCbCr");
    auto const range = enum_value(find_property(drm_fd, props, "COLOR_RANGE"), "YCbCr limited range");
    if (!encoding || !range)
        return std::nullopt;
    return mga::Bt709Limited{*encoding, *range};
}

template<typename T>
auto read_at(std::span<std::byte const> data, size_t offset) -> T
{
    T value{};
    std::memcpy(&value, data.data() + offset, sizeof value);
    return value;
}

auto within(size_t size, uint32_t offset, uint32_t count, size_t element_size) -> bool
{
    return uint64_t{offset} + uint64_t{count} * element_size <= size;
}

auto parse_in_formats(std::span<std::byte const> data) -> std::optional<std::vector<mga::FormatModifier>>
{
    if (data.size() < sizeof(drm_format_modifier_blob))
        return std::nullopt;

    auto const header = read_at<drm_format_modifier_blob>(data, 0);
    if (header.version != FORMAT_BLOB_CURRENT ||
        !within(data.size(), header.formats_offset, header.count_formats, sizeof(uint32_t)) ||
        !within(data.size(), header.modifiers_offset, header.count_modifiers, sizeof(drm_format_modifier)))
        return std::nullopt;

    std::vector<mga::FormatModifier> pairs;
    for (uint32_t i = 0; i != header.count_modifiers; ++i)
    {
        auto const entry = read_at<drm_format_modifier>(
            data, header.modifiers_offset + i * sizeof(drm_format_modifier));
        for (uint64_t bit = 0; bit < 64; ++bit)
        {
            if ((entry.formats & (uint64_t{1} << bit)) == 0)
                continue;

            auto const index = uint64_t{entry.offset} + bit;
            if (index >= header.count_formats)
                return std::nullopt;
            pairs.push_back({
                read_at<uint32_t>(data, header.formats_offset + index * sizeof(uint32_t)), entry.modifier});
        }
    }

    if (pairs.empty())
        return std::nullopt;
    return pairs;
}

auto decode_formats(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<std::vector<mga::FormatModifier>>
{
    if (!props.has_property("IN_FORMATS"))
        return std::nullopt;

    auto const blob_id = props["IN_FORMATS"];
    if (blob_id == 0 || blob_id > std::numeric_limits<uint32_t>::max())
        return std::nullopt;

    BlobUPtr const blob{drmModeGetPropertyBlob(drm_fd, static_cast<uint32_t>(blob_id)), &drmModeFreePropertyBlob};
    if (!blob)
        return std::nullopt;

    return parse_in_formats({static_cast<std::byte const*>(blob->data), blob->length});
}

auto decode_plane(int drm_fd, mgk::ObjectProperties const& props) -> std::optional<mga::PlaneCapabilities>
{
    auto formats = decode_formats(drm_fd, props);
    if (!formats)
        return std::nullopt;

    std::optional<mga::PropertyValue> blend_none;
    if (auto const blend = find_property(drm_fd, props, "pixel blend mode"))
    {
        blend_none = enum_value(blend, "None");
        if (!blend_none)
            return std::nullopt;
    }

    auto const alpha_max = decode_alpha_max(drm_fd, props);
    return mga::PlaneCapabilities{
        .id = props.parent_id(),
        .zpos = decode_zpos(drm_fd, props),
        .formats = std::move(*formats),
        .alpha_max = alpha_max,
        .blend_none = blend_none,
        .fixed_opaque = !(alpha_max && blend_none),
        .bt709_limited = decode_bt709_limited(drm_fd, props),
        .can_be_above_primary = false,
        .can_be_below_primary = false,
    };
}

void assign_stacking(mga::PlaneCapabilities const& primary, std::vector<mga::PlaneCapabilities>& overlays)
{
    for (auto& overlay : overlays)
    {
        auto const& zpos = *overlay.zpos;
        overlay.can_be_above_primary = !primary.zpos || zpos.max > primary.zpos->current;
        overlay.can_be_below_primary = primary.zpos && zpos.min < primary.zpos->current;
    }
}
}

auto mga::discover_plane_capabilities(int drm_fd, uint32_t crtc_id) -> CrtcPlaneCapabilities
{
    auto const crtc_index = crtc_index_of(drm_fd, crtc_id);
    auto const crtc_mask = uint32_t{1} << crtc_index;

    std::optional<PlaneCapabilities> primary;
    bool primary_bound{false};
    std::vector<PlaneCapabilities> overlays;

    mgk::PlaneResources const plane_resources{drm_fd};
    for (auto const& plane : plane_resources.planes())
    {
        if ((plane->possible_crtcs & crtc_mask) == 0)
            continue;

        mgk::ObjectProperties const props{drm_fd, plane};
        auto const type = props.has_property("type") ? props["type"] : std::numeric_limits<uint64_t>::max();
        switch (type)
        {
        case DRM_PLANE_TYPE_PRIMARY:
        {
            auto candidate = decode_plane(drm_fd, props);
            bool const bound = plane->crtc_id == crtc_id;
            if (candidate && (!primary || (bound && !primary_bound)))
            {
                primary = std::move(candidate);
                primary_bound = bound;
            }
            break;
        }
        case DRM_PLANE_TYPE_OVERLAY:
            if (auto overlay = decode_plane(drm_fd, props); overlay && overlay->zpos)
                overlays.push_back(std::move(*overlay));
            break;
        case DRM_PLANE_TYPE_CURSOR:
            break;
        default:
            mir::log_warning("Ignoring plane %u with missing or unknown type", props.parent_id());
            break;
        }
    }

    if (!primary)
        BOOST_THROW_EXCEPTION(std::runtime_error{"No usable primary plane for CRTC " + std::to_string(crtc_id)});

    assign_stacking(*primary, overlays);
    std::ranges::sort(
        overlays,
        {},
        [](PlaneCapabilities const& overlay) { return std::pair{overlay.zpos->current, overlay.id}; });

    std::string fixed_opaque_ids = primary->fixed_opaque ? std::to_string(primary->id) : "";
    for (auto const& overlay : overlays)
    {
        if (overlay.fixed_opaque)
            fixed_opaque_ids += (fixed_opaque_ids.empty() ? "" : ", ") + std::to_string(overlay.id);
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
        .crtc_index = crtc_index,
        .primary = std::move(*primary),
        .overlays = std::move(overlays),
    };
}

void mga::PlaneCapabilityState::rebuild(int drm_fd, uint32_t crtc_id)
{
    *state.lock() = nullptr;

    try
    {
        auto caps = std::make_shared<CrtcPlaneCapabilities const>(discover_plane_capabilities(drm_fd, crtc_id));
        *state.lock() = std::move(caps);
    }
    catch (std::exception const& e)
    {
        mir::log_warning("Failed to discover plane capabilities for CRTC %u: %s", crtc_id, e.what());
    }
}

auto mga::PlaneCapabilityState::current() const -> std::shared_ptr<CrtcPlaneCapabilities const>
{
    return *state.lock();
}
