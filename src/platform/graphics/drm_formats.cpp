/*
 * Copyright © 2022 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/graphics/drm_formats.h"

#include <cstdint>
#include <xf86drm.h>
#include <drm_fourcc.h>
#include <unordered_map>
#include <stdexcept>

namespace mg = mir::graphics;

namespace
{
constexpr auto drm_format_to_string(uint32_t format) -> char const*
{
#define STRINGIFY(val) \
    case val:          \
        return #val;

    if (!(format & DRM_FORMAT_BIG_ENDIAN))
    {
        switch (format)
        {
#include "drm-formats"

            default:
                return "Unknown DRM format; rebuild Mir against newer DRM headers?";
        }

    }
#undef STRINGIFY

#define STRINGIFY_BIG_ENDIAN(val) \
    case val:                    \
        return #val " (big endian)";

    switch (format & (~DRM_FORMAT_BIG_ENDIAN))
    {
#include "drm-formats-big-endian"

        default:
            return "Unknown DRM format; rebuild Mir against newer DRM headers?";
    }
#undef STRINGIFY_BIGENDIAN
}


struct FormatInfo
{
    uint32_t format;
    bool has_alpha;
    uint32_t opaque_equivalent;
    uint32_t alpha_equivalent;
    std::optional<mg::DRMFormat::RGBComponentInfo> components;
};

std::unordered_map<uint32_t, FormatInfo const> const formats = {
    {
        DRM_FORMAT_XRGB4444,
        {
            DRM_FORMAT_XRGB4444,
            false,
            DRM_FORMAT_XRGB4444,
            DRM_FORMAT_ARGB4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, {}
            },
        }
    },
    {
        DRM_FORMAT_XBGR4444,
        {
            DRM_FORMAT_XBGR4444,
            false,
            DRM_FORMAT_XBGR4444,
            DRM_FORMAT_ABGR4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, {}
            },
        }
    },
    {
        DRM_FORMAT_RGBX4444,
        {
            DRM_FORMAT_RGBX4444,
            false,
            DRM_FORMAT_RGBX4444,
            DRM_FORMAT_RGBA4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, {}
            },
        }
    },
    {
        DRM_FORMAT_BGRX4444,
        {
            DRM_FORMAT_BGRX4444,
            false,
            DRM_FORMAT_BGRX4444,
            DRM_FORMAT_BGRA4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, {}
            },
        }
    },
    {
        DRM_FORMAT_ARGB4444,
        {
            DRM_FORMAT_ARGB4444,
            true,
            DRM_FORMAT_XRGB4444,
            DRM_FORMAT_ARGB4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, 4
            },
        }
    },
    {
        DRM_FORMAT_ABGR4444,
        {
            DRM_FORMAT_ABGR4444,
            true,
            DRM_FORMAT_XBGR4444,
            DRM_FORMAT_ABGR4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, 4
            },
        }
    },
    {
        DRM_FORMAT_RGBA4444,
        {
            DRM_FORMAT_RGBA4444,
            true,
            DRM_FORMAT_RGBX4444,
            DRM_FORMAT_RGBA4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, 4
            },
        }
    },
    {
        DRM_FORMAT_BGRA4444,
        {
            DRM_FORMAT_BGRA4444,
            true,
            DRM_FORMAT_BGRX4444,
            DRM_FORMAT_BGRA4444,
            mg::DRMFormat::RGBComponentInfo{
                4, 4, 4, 4
            },
        }
    },
    {
        DRM_FORMAT_XRGB1555,
        {
            DRM_FORMAT_XRGB1555,
            false,
            DRM_FORMAT_XRGB1555,
            DRM_FORMAT_ARGB1555,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, {}
            },
        }
    },
    {
        DRM_FORMAT_XBGR1555,
        {
            DRM_FORMAT_XBGR1555,
            false,
            DRM_FORMAT_XBGR1555,
            DRM_FORMAT_ABGR1555,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, {}
            },
        }
    },
    {
        DRM_FORMAT_RGBX5551,
        {
            DRM_FORMAT_RGBX5551,
            false,
            DRM_FORMAT_RGBX5551,
            DRM_FORMAT_RGBA5551,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, {}
            },
        }
    },
    {
        DRM_FORMAT_BGRX5551,
        {
            DRM_FORMAT_BGRX5551,
            false,
            DRM_FORMAT_BGRX5551,
            DRM_FORMAT_BGRA5551,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, {}
            },
        }
    },
    {
        DRM_FORMAT_ARGB1555,
        {
            DRM_FORMAT_ARGB1555,
            true,
            DRM_FORMAT_XRGB1555,
            DRM_FORMAT_ARGB1555,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, 1
            },
        }
    },
    {
        DRM_FORMAT_ABGR1555,
        {
            DRM_FORMAT_ABGR1555,
            true,
            DRM_FORMAT_XBGR1555,
            DRM_FORMAT_ABGR1555,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, 1
            },
        }
    },
    {
        DRM_FORMAT_RGBA5551,
        {
            DRM_FORMAT_RGBA5551,
            true,
            DRM_FORMAT_RGBX5551,
            DRM_FORMAT_RGBA5551,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, 1
            },
        }
    },
    {
        DRM_FORMAT_BGRA5551,
        {
            DRM_FORMAT_BGRA5551,
            true,
            DRM_FORMAT_BGRX5551,
            DRM_FORMAT_BGRA5551,
            mg::DRMFormat::RGBComponentInfo{
                5, 5, 5, 1
            },
        }
    },
    {
        DRM_FORMAT_RGB565,
        {
            DRM_FORMAT_RGB565,
            false,
            DRM_FORMAT_RGB565,
            DRM_FORMAT_INVALID,
            mg::DRMFormat::RGBComponentInfo{
                5, 6, 5, {}
            },
        }
    },
    {
        DRM_FORMAT_BGR565,
        {
            DRM_FORMAT_BGR565,
            false,
            DRM_FORMAT_BGR565,
            DRM_FORMAT_INVALID,
            mg::DRMFormat::RGBComponentInfo{
                5, 6, 5, {}
            },
        }
    },
    {
        DRM_FORMAT_RGB888,
        {
            DRM_FORMAT_RGB888,
            false,
            DRM_FORMAT_RGB888,
            DRM_FORMAT_INVALID,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, {}
            },
        }
    },
    {
        DRM_FORMAT_BGR888,
        {
            DRM_FORMAT_BGR888,
            false,
            DRM_FORMAT_BGR888,
            DRM_FORMAT_INVALID,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, {}
            },
        }
    },
    {
        DRM_FORMAT_XRGB8888,
        {
            DRM_FORMAT_XRGB8888,
            false,
            DRM_FORMAT_XRGB8888,
            DRM_FORMAT_ARGB8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, {}
            },
        }
    },
    {
        DRM_FORMAT_XBGR8888,
        {
            DRM_FORMAT_XBGR8888,
            false,
            DRM_FORMAT_XBGR8888,
            DRM_FORMAT_ABGR8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, {}
            },
        }
    },
    {
        DRM_FORMAT_RGBX8888,
        {
            DRM_FORMAT_RGBX8888,
            false,
            DRM_FORMAT_RGBX8888,
            DRM_FORMAT_RGBA8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, {}
            },
        }
    },
    {
        DRM_FORMAT_BGRX8888,
        {
            DRM_FORMAT_BGRX8888,
            false,
            DRM_FORMAT_BGRX8888,
            DRM_FORMAT_BGRA8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, {}
            },
        }
    },
    {
        DRM_FORMAT_ARGB8888,
        {
            DRM_FORMAT_ARGB8888,
            true,
            DRM_FORMAT_XRGB8888,
            DRM_FORMAT_ARGB8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, 8
            },
        }
    },
    {
        DRM_FORMAT_ABGR8888,
        {
            DRM_FORMAT_ABGR8888,
            true,
            DRM_FORMAT_XBGR8888,
            DRM_FORMAT_ABGR8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, 8
            },
        }
    },
    {
        DRM_FORMAT_RGBA8888,
        {
            DRM_FORMAT_RGBA8888,
            true,
            DRM_FORMAT_RGBX8888,
            DRM_FORMAT_RGBA8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, 8
            },
        }
    },
    {
        DRM_FORMAT_BGRA8888,
        {
            DRM_FORMAT_BGRA8888,
            true,
            DRM_FORMAT_BGRX8888,
            DRM_FORMAT_BGRA8888,
            mg::DRMFormat::RGBComponentInfo{
                8, 8, 8, 8
            },
        }
    },
    {
        DRM_FORMAT_XRGB2101010,
        {
            DRM_FORMAT_XRGB2101010,
            false,
            DRM_FORMAT_XRGB2101010,
            DRM_FORMAT_ARGB2101010,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, {}
            },
        }
    },
    {
        DRM_FORMAT_XBGR2101010,
        {
            DRM_FORMAT_XBGR2101010,
            false,
            DRM_FORMAT_XBGR2101010,
            DRM_FORMAT_ABGR2101010,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, {}
            },
        }
    },
    {
        DRM_FORMAT_RGBX1010102,
        {
            DRM_FORMAT_RGBX1010102,
            false,
            DRM_FORMAT_RGBX1010102,
            DRM_FORMAT_RGBA1010102,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, {}
            },
        }
    },
    {
        DRM_FORMAT_BGRX1010102,
        {
            DRM_FORMAT_BGRX1010102,
            false,
            DRM_FORMAT_BGRX1010102,
            DRM_FORMAT_BGRA1010102,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, {}
            },
        }
    },
    {
        DRM_FORMAT_ARGB2101010,
        {
            DRM_FORMAT_ARGB2101010,
            true,
            DRM_FORMAT_XRGB2101010,
            DRM_FORMAT_ARGB2101010,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, 2
            },
        }
    },
    {
        DRM_FORMAT_ABGR2101010,
        {
            DRM_FORMAT_ABGR2101010,
            true,
            DRM_FORMAT_XBGR2101010,
            DRM_FORMAT_ABGR2101010,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, 2
            },
        }
    },
    {
        DRM_FORMAT_RGBA1010102,
        {
            DRM_FORMAT_RGBA1010102,
            true,
            DRM_FORMAT_RGBX1010102,
            DRM_FORMAT_RGBA1010102,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, 2
            },
        }
    },
    {
        DRM_FORMAT_BGRA1010102,
        {
            DRM_FORMAT_BGRA1010102,
            true,
            DRM_FORMAT_BGRX1010102,
            DRM_FORMAT_BGRA1010102,
            mg::DRMFormat::RGBComponentInfo{
                10, 10, 10, 2
            },
        }
    },
};
}

auto mg::DRMFormat::name() const -> const char*
{
    return drm_format_to_string(format);
}

auto mg::DRMFormat::opaque_equivalent() const -> const std::optional<DRMFormat>
{
    auto const opaque_format = formats.at(format).opaque_equivalent;
    if (opaque_format != DRM_FORMAT_INVALID)
    {
        return DRMFormat{opaque_format};
    }
    return {};
}

auto mg::DRMFormat::alpha_equivalent() const -> const std::optional<DRMFormat>
{
    auto const opaque_format = formats.at(format).alpha_equivalent;
    if (opaque_format != DRM_FORMAT_INVALID)
    {
        return DRMFormat{opaque_format};
    }
    return {};
}

bool mg::DRMFormat::has_alpha() const
{
    return formats.at(format).has_alpha;
}

auto mg::DRMFormat::components() const -> std::optional<RGBComponentInfo> const&
{
    return formats.at(format).components;
}

mg::DRMFormat::operator uint32_t() const
{
    return format;
}

auto mg::drm_modifier_to_string(uint64_t modifier) -> std::string
{
#ifdef HAVE_DRM_GET_MODIFIER_NAME
    struct CStrDeleter
    {
    public:
        void operator()(char* c_str)
        {
            if (c_str)
            {
                free (c_str);
            }
        }
    };

    auto const vendor =
        [&]() -> std::string
        {
            std::unique_ptr<char[], CStrDeleter> vendor{drmGetFormatModifierVendor(modifier)};
            if (vendor)
            {
                return vendor.get();
            }
            return "(UNKNOWN VENDOR)";
        }();

    auto const name =
        [&]() -> std::string
        {
            std::unique_ptr<char[], CStrDeleter> name{drmGetFormatModifierName(modifier)};
            if (name)
            {
                return name.get();
            }
            return "(UNKNOWN MODIFIER)";
        }();

    return vendor + ":" + name;
}
#else
#define STRINGIFY(val) \
    case val:          \
        return #val;

    switch (modifier)
    {
#ifdef DRM_FORMAT_MOD_INVALID
        STRINGIFY(DRM_FORMAT_MOD_INVALID)
#endif
#ifdef DRM_FORMAT_MOD_LINEAR
        STRINGIFY(DRM_FORMAT_MOD_LINEAR)
#endif
#ifdef I915_FORMAT_MOD_X_TILED
        STRINGIFY(I915_FORMAT_MOD_X_TILED)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED
        STRINGIFY(I915_FORMAT_MOD_Y_TILED)
#endif
#ifdef I915_FORMAT_MOD_Yf_TILED
        STRINGIFY(I915_FORMAT_MOD_Yf_TILED)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED_CCS
        STRINGIFY(I915_FORMAT_MOD_Y_TILED_CCS)
#endif
#ifdef I915_FORMAT_MOD_Yf_TILED_CCS
        STRINGIFY(I915_FORMAT_MOD_Yf_TILED_CCS)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS
        STRINGIFY(I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS
        STRINGIFY(I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS)
#endif
#ifdef DRM_FORMAT_MOD_SAMSUNG_64_32_TILE
        STRINGIFY(DRM_FORMAT_MOD_SAMSUNG_64_32_TILE)
#endif
#ifdef DRM_FORMAT_MOD_SAMSUNG_16_16_TILE
        STRINGIFY(DRM_FORMAT_MOD_SAMSUNG_16_16_TILE)
#endif
#ifdef DRM_FORMAT_MOD_QCOM_COMPRESSED
        STRINGIFY(DRM_FORMAT_MOD_QCOM_COMPRESSED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_TILED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_SUPER_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_SUPER_TILED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_ONE_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_ONE_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_TWO_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_TWO_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_FOUR_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_FOUR_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_EIGHT_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_EIGHT_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_SIXTEEN_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_SIXTEEN_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_THIRTYTWO_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_THIRTYTWO_GOB)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND32
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND32)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND64
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND64)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND128
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND128)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND256
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND256)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_UIF
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_UIF)
#endif
#ifdef DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED
        STRINGIFY(DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED)
#endif
#ifdef DRM_FORMAT_MOD_ALLWINNER_TILED
        STRINGIFY(DRM_FORMAT_MOD_ALLWINNER_TILED)
#endif

        default:
            return "(unknown)";
    }

#undef STRINGIFY
}
#endif

