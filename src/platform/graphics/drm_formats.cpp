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
 */

#include "mir/graphics/drm_formats.h"

#include <cstdint>
#include <drm_fourcc.h>
#include <stdexcept>
#include <memory>
#include <array>
#include <boost/throw_exception.hpp>

#ifdef MIR_HAVE_DRM_GET_MODIFIER_NAME
#include <xf86drm.h>
#endif

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

}
struct mg::DRMFormat::FormatInfo
{
    uint32_t format;
    bool has_alpha;
    uint32_t opaque_equivalent;
    uint32_t alpha_equivalent;
    std::optional<mg::DRMFormat::RGBComponentInfo> components;
};

namespace
{
constexpr std::array const formats = {
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XRGB4444,
        false,
        DRM_FORMAT_XRGB4444,
        DRM_FORMAT_ARGB4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XBGR4444,
        false,
        DRM_FORMAT_XBGR4444,
        DRM_FORMAT_ABGR4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBX4444,
        false,
        DRM_FORMAT_RGBX4444,
        DRM_FORMAT_RGBA4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRX4444,
        false,
        DRM_FORMAT_BGRX4444,
        DRM_FORMAT_BGRA4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ARGB4444,
        true,
        DRM_FORMAT_XRGB4444,
        DRM_FORMAT_ARGB4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, 4
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ABGR4444,
        true,
        DRM_FORMAT_XBGR4444,
        DRM_FORMAT_ABGR4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, 4
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBA4444,
        true,
        DRM_FORMAT_RGBX4444,
        DRM_FORMAT_RGBA4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, 4
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRA4444,
        true,
        DRM_FORMAT_BGRX4444,
        DRM_FORMAT_BGRA4444,
        mg::DRMFormat::RGBComponentInfo{
            4, 4, 4, 4
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XRGB1555,
        false,
        DRM_FORMAT_XRGB1555,
        DRM_FORMAT_ARGB1555,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XBGR1555,
        false,
        DRM_FORMAT_XBGR1555,
        DRM_FORMAT_ABGR1555,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBX5551,
        false,
        DRM_FORMAT_RGBX5551,
        DRM_FORMAT_RGBA5551,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRX5551,
        false,
        DRM_FORMAT_BGRX5551,
        DRM_FORMAT_BGRA5551,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ARGB1555,
        true,
        DRM_FORMAT_XRGB1555,
        DRM_FORMAT_ARGB1555,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, 1
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ABGR1555,
        true,
        DRM_FORMAT_XBGR1555,
        DRM_FORMAT_ABGR1555,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, 1
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBA5551,
        true,
        DRM_FORMAT_RGBX5551,
        DRM_FORMAT_RGBA5551,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, 1
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRA5551,
        true,
        DRM_FORMAT_BGRX5551,
        DRM_FORMAT_BGRA5551,
        mg::DRMFormat::RGBComponentInfo{
            5, 5, 5, 1
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGB565,
        false,
        DRM_FORMAT_RGB565,
        DRM_FORMAT_INVALID,
        mg::DRMFormat::RGBComponentInfo{
            5, 6, 5, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGR565,
        false,
        DRM_FORMAT_BGR565,
        DRM_FORMAT_INVALID,
        mg::DRMFormat::RGBComponentInfo{
            5, 6, 5, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGB888,
        false,
        DRM_FORMAT_RGB888,
        DRM_FORMAT_INVALID,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGR888,
        false,
        DRM_FORMAT_BGR888,
        DRM_FORMAT_INVALID,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XRGB8888,
        false,
        DRM_FORMAT_XRGB8888,
        DRM_FORMAT_ARGB8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XBGR8888,
        false,
        DRM_FORMAT_XBGR8888,
        DRM_FORMAT_ABGR8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBX8888,
        false,
        DRM_FORMAT_RGBX8888,
        DRM_FORMAT_RGBA8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRX8888,
        false,
        DRM_FORMAT_BGRX8888,
        DRM_FORMAT_BGRA8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ARGB8888,
        true,
        DRM_FORMAT_XRGB8888,
        DRM_FORMAT_ARGB8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, 8
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ABGR8888,
        true,
        DRM_FORMAT_XBGR8888,
        DRM_FORMAT_ABGR8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, 8
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBA8888,
        true,
        DRM_FORMAT_RGBX8888,
        DRM_FORMAT_RGBA8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, 8
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRA8888,
        true,
        DRM_FORMAT_BGRX8888,
        DRM_FORMAT_BGRA8888,
        mg::DRMFormat::RGBComponentInfo{
            8, 8, 8, 8
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XRGB2101010,
        false,
        DRM_FORMAT_XRGB2101010,
        DRM_FORMAT_ARGB2101010,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_XBGR2101010,
        false,
        DRM_FORMAT_XBGR2101010,
        DRM_FORMAT_ABGR2101010,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBX1010102,
        false,
        DRM_FORMAT_RGBX1010102,
        DRM_FORMAT_RGBA1010102,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRX1010102,
        false,
        DRM_FORMAT_BGRX1010102,
        DRM_FORMAT_BGRA1010102,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, {}
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ARGB2101010,
        true,
        DRM_FORMAT_XRGB2101010,
        DRM_FORMAT_ARGB2101010,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, 2
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_ABGR2101010,
        true,
        DRM_FORMAT_XBGR2101010,
        DRM_FORMAT_ABGR2101010,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, 2
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_RGBA1010102,
        true,
        DRM_FORMAT_RGBX1010102,
        DRM_FORMAT_RGBA1010102,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, 2
        },
    },
    mg::DRMFormat::FormatInfo{
        DRM_FORMAT_BGRA1010102,
        true,
        DRM_FORMAT_BGRX1010102,
        DRM_FORMAT_BGRA1010102,
        mg::DRMFormat::RGBComponentInfo{
            10, 10, 10, 2
        },
    },
};

constexpr auto find_format_info(uint32_t fourcc) -> mg::DRMFormat::FormatInfo const*
{
    for (auto const& format: formats)
    {
        if (format.format == fourcc)
            return &format;
    }
    /* The format array doesn't cover all DRM_FORMAT_*, only the ones relevant to Mir
     * (and not all of them, yet: eg YUV formats), so we must have a sentinel value
     * for the missing ones
     */
    return nullptr;
}

/* Generate a bunch of variables named format_info_DRM_FORMAT_FOO pointing to associated FormatInfo */
#define STRINGIFY(format) \
    constexpr mg::DRMFormat::FormatInfo const* format_info_##format = find_format_info(format);

#include "drm-formats"

#undef STRINGIFY

constexpr auto maybe_info_for_format(uint32_t fourcc_format) -> mg::DRMFormat::FormatInfo const*
{
    switch (fourcc_format)
    {
#define STRINGIFY(format) \
    case format: \
        return format_info_##format; \

#include "drm-formats"

#undef STRINGIFY
        default:
            BOOST_THROW_EXCEPTION((
                std::runtime_error{
                    std::string{"Unknown DRM format "} + std::to_string(fourcc_format) +
                    " (may need to rebuild Mir against newer DRM headers?)"}));
    }

}

constexpr auto info_for_format(uint32_t fourcc_format) -> mg::DRMFormat::FormatInfo const&
{
    auto const info = maybe_info_for_format(fourcc_format);

    if (info)
    {
        return *info;
    }
    BOOST_THROW_EXCEPTION((
        std::runtime_error{
            std::string{"Unsupported DRM format: "} + drm_format_to_string(fourcc_format)}));
}

}

mg::DRMFormat::DRMFormat(uint32_t fourcc_format)
    : info{&info_for_format(fourcc_format)}
{
}

auto mg::DRMFormat::name() const -> const char*
{
    return drm_format_to_string(info->format);
}

auto mg::DRMFormat::opaque_equivalent() const -> const std::optional<DRMFormat>
{
    if (info->opaque_equivalent != DRM_FORMAT_INVALID)
    {
        return DRMFormat{info->opaque_equivalent};
    }
    return {};
}

auto mg::DRMFormat::alpha_equivalent() const -> const std::optional<DRMFormat>
{
    if (info->alpha_equivalent != DRM_FORMAT_INVALID)
    {
        return DRMFormat{info->alpha_equivalent};
    }
    return {};
}

bool mg::DRMFormat::has_alpha() const
{
    return info->has_alpha;
}

auto mg::DRMFormat::components() const -> std::optional<RGBComponentInfo> const&
{
    return info->components;
}

mg::DRMFormat::operator uint32_t() const
{
    return info->format;
}

auto mg::DRMFormat::as_mir_format() const -> std::optional<MirPixelFormat>
{
    switch (info->format)
    {
    case DRM_FORMAT_ARGB8888:
        return mir_pixel_format_argb_8888;
    case DRM_FORMAT_XRGB8888:
        return mir_pixel_format_xrgb_8888;
    case DRM_FORMAT_RGBA4444:
        return mir_pixel_format_rgba_4444;
    case DRM_FORMAT_RGBA5551:
        return mir_pixel_format_rgba_5551;
    case DRM_FORMAT_RGB565:
        return mir_pixel_format_rgb_565;
    case DRM_FORMAT_RGB888:
        return mir_pixel_format_rgb_888;
    case DRM_FORMAT_BGR888:
        return mir_pixel_format_bgr_888;
    case DRM_FORMAT_XBGR8888:
        return mir_pixel_format_xbgr_8888;
    case DRM_FORMAT_ABGR8888:
        return mir_pixel_format_abgr_8888;
    default:
        return {};
    }
}    

auto mg::drm_modifier_to_string(uint64_t modifier) -> std::string
{
#ifdef MIR_HAVE_DRM_GET_MODIFIER_NAME
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

