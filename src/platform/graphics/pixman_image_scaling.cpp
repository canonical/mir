/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/graphics/pixman_image_scaling.h"

#include "mir/graphics/cursor_image.h"
#include "mir_toolkit/common.h"

#include "pixman-1/pixman.h"

#include <cmath>
#include <cstring>

mir::graphics::ARGB8Buffer mir::graphics::scale_cursor_image(
    mir::graphics::CursorImage const& cursor_image, float new_scale)
{
    auto const [width, height] = cursor_image.size();

    // Special case: no resizing. Just copy the data.
    if(new_scale == 1.0f)
    {
        auto const pixel_count = width.as_value() * height.as_value();
        auto buf = std::make_unique<uint32_t[]>(pixel_count);
        std::memcpy(buf.get(), cursor_image.as_argb_8888(), pixel_count * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888));
        return {std::move(buf), {width, height}};
    }

    using UniquePixmanImage = std::unique_ptr<pixman_image_t, decltype(&pixman_image_unref)>;

    auto original_image = UniquePixmanImage(
        pixman_image_create_bits(
            PIXMAN_a8r8g8b8,
            width.as_int(),
            height.as_int(),
            (uint32_t*)(cursor_image.as_argb_8888()),
            width.as_int() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888)),
        &pixman_image_unref);

    pixman_transform_t transform;
    pixman_transform_init_identity(&transform);
    // Counter-intuitively, you use the INVERSE transform, not the "normal" one
    pixman_transform_scale(NULL, &transform, pixman_double_to_fixed(new_scale), pixman_double_to_fixed(new_scale));
    pixman_image_set_transform(original_image.get(), &transform);
    pixman_image_set_filter(original_image.get(), PIXMAN_FILTER_BILINEAR, NULL, 0);

    // Round the width and height up/down to the nearest integer. If the width
    // stays as a float, the stride might be computed as not a value of 4
    // depending on the scale. Causing the assert linked below to blow up.
    // https://gitlab.freedesktop.org/pixman/pixman/-/blob/master/pixman/pixman-bits-image.c#L1339
    int const scaled_width = std::round(width.as_value() * new_scale);
    int const scaled_height = std::round(height.as_value() * new_scale);

    auto buf = std::make_unique<uint32_t[]>(scaled_width * scaled_height);
    auto scaled_image = UniquePixmanImage(
        pixman_image_create_bits(
            PIXMAN_a8r8g8b8,
            scaled_width,
            scaled_height,
            buf.get(),
            scaled_width * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888)),
        &pixman_image_unref);

    // Copy over the pixels into the larger image
    pixman_image_composite(
        PIXMAN_OP_SRC, original_image.get(), NULL, scaled_image.get(), 0, 0, 0, 0, 0, 0, scaled_width, scaled_height);

    return {std::move(buf), {scaled_width, scaled_height}};
}
