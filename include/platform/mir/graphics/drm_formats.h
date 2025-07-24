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

#ifndef MIR_PLATFORM_GRAPHICS_DRM_FORMATS_H_
#define MIR_PLATFORM_GRAPHICS_DRM_FORMATS_H_

#include "mir_toolkit/common.h"
#include <cstdint>
#include <string>
#include <optional>

namespace mir::graphics
{
class DRMFormat
{
public:
    struct FormatInfo;
    class Info
    {
    public:
        struct RGBComponentInfo
        {
            uint32_t red_bits;
            uint32_t green_bits;
            uint32_t blue_bits;
            std::optional<uint32_t> alpha_bits;
        };

        auto opaque_equivalent() const -> std::optional<DRMFormat>;
        auto alpha_equivalent() const -> std::optional<DRMFormat>;

        bool has_alpha() const;

        auto components() const -> std::optional<RGBComponentInfo> const&;
        auto bits_per_pixel() const -> std::optional<uint32_t>;
    private:
        friend class DRMFormat;
        Info(FormatInfo const* info);

        FormatInfo const* info;
    };

    constexpr explicit DRMFormat(uint32_t fourcc_format)
        : fourcc{fourcc_format}
    {
    }

    auto name() const -> char const*;

    auto info() const -> std::optional<Info const>;

    operator uint32_t() const;

    auto as_mir_format() const -> std::optional<MirPixelFormat>;
    static auto from_mir_format(MirPixelFormat format) -> DRMFormat;

private:
    uint32_t fourcc;
};

auto drm_modifier_to_string(uint64_t modifier) -> std::string;
auto drm_format_to_string(uint32_t format) -> char const*;
}

#endif //MIR_PLATFORM_GRAPHICS_DRM_FORMATS_H_
