/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "cursor.h"
#include "platform.h"
#include "kms_output.h"
#include "kms_output_container.h"
#include "kms_display_configuration.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/cursor_image.h"

#include <xf86drm.h>

#include <boost/exception/errinfo_errno.hpp>

#include <stdexcept>
#include <vector>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace geom = mir::geometry;

namespace
{
const uint64_t fallback_cursor_size = 64;
char const* const mir_drm_cursor_64x64 = "MIR_DRM_CURSOR_64x64";

// Transforms a relative position within the display bounds described by \a rect which is rotated with \a orientation
geom::Displacement transform(geom::Rectangle const& rect, geom::Displacement const& vector, MirOrientation orientation)
{
    switch(orientation)
    {
    case mir_orientation_left:
        return {vector.dy.as_int(), rect.size.width.as_int() -vector.dx.as_int()};
    case mir_orientation_inverted:
        return {rect.size.width.as_int() -vector.dx.as_int(), rect.size.height.as_int() - vector.dy.as_int()};
    case mir_orientation_right:
        return {rect.size.height.as_int() -vector.dy.as_int(), vector.dx.as_int()};
    default:
    case mir_orientation_normal:
        return vector;
    }
}
// support for older drm headers
#ifndef DRM_CAP_CURSOR_WIDTH
#define DRM_CAP_CURSOR_WIDTH            0x8
#define DRM_CAP_CURSOR_HEIGHT           0x9
#endif

// In certain combinations of DRI backends and drivers GBM
// returns a stride size that matches the requested buffers size,
// instead of the underlying buffer:
// https://bugs.freedesktop.org/show_bug.cgi?id=89164
int get_drm_cursor_height(int fd)
{
    // on some older hardware drm incorrectly reports the cursor size
    bool const force_64x64_cursor = getenv(mir_drm_cursor_64x64);

    uint64_t height = fallback_cursor_size;
    if (!force_64x64_cursor)
       drmGetCap(fd, DRM_CAP_CURSOR_HEIGHT, &height);
    return int(height);
}

int get_drm_cursor_width(int fd)
{
    // on some older hardware drm incorrectly reports the cursor size
    bool const force_64x64_cursor = getenv(mir_drm_cursor_64x64);

    uint64_t width = fallback_cursor_size;
    if (!force_64x64_cursor)
       drmGetCap(fd, DRM_CAP_CURSOR_WIDTH, &width);
    return int(width);
}

gbm_device* gbm_create_device_checked(int fd)
{
    auto device = gbm_create_device(fd);
    if (!device)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create gbm device"));
    }
    return device;
}
}

mgm::Cursor::GBMBOWrapper::GBMBOWrapper(int fd, MirOrientation orientation) :
    device{gbm_create_device_checked(fd)},
    buffer{
        gbm_bo_create(
            device,
            get_drm_cursor_width(fd),
            get_drm_cursor_height(fd),
            GBM_FORMAT_ARGB8888,
            GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE)},
    current_orientation{orientation}
{
    if (!buffer) BOOST_THROW_EXCEPTION(std::runtime_error("failed to create gbm buffer"));
}

inline mgm::Cursor::GBMBOWrapper::operator gbm_bo*()
{
    return buffer;
}

inline mgm::Cursor::GBMBOWrapper::~GBMBOWrapper()
{
    if (device)
        gbm_device_destroy(device);
    if (buffer)
        gbm_bo_destroy(buffer);
}

mgm::Cursor::GBMBOWrapper::GBMBOWrapper(GBMBOWrapper&& from)
    : device{from.device},
      buffer{from.buffer},
      current_orientation{from.current_orientation}
{
    from.buffer = nullptr;
    from.device = nullptr;
}

auto mir::graphics::mesa::Cursor::GBMBOWrapper::change_orientation(MirOrientation new_orientation) -> bool
{
    if (current_orientation == new_orientation)
        return false;

    current_orientation = new_orientation;
    return true;
}

mgm::Cursor::Cursor(
    KMSOutputContainer& output_container,
    std::shared_ptr<CurrentConfiguration> const& current_configuration) :
        output_container(output_container),
        current_position(),
        last_set_failed(false),
        min_buffer_width{std::numeric_limits<uint32_t>::max()},
        min_buffer_height{std::numeric_limits<uint32_t>::max()},
        current_configuration(current_configuration)
{
    // Generate the buffers for the initial configuration.
    current_configuration->with_current_configuration_do(
        [this](KMSDisplayConfiguration const& kms_conf)
        {
            kms_conf.for_each_output(
                [this, &kms_conf](auto const& output)
                {
                    // I'm not sure why g++ needs the explicit "this->" but it does - alan_g
                    this->buffer_for_output(*kms_conf.get_output_for(output.id));
                });
        });

    hide();
    if (last_set_failed)
        throw std::runtime_error("Initial KMS cursor set failed");
}

mgm::Cursor::~Cursor() noexcept
{
    hide();
}

void mgm::Cursor::write_buffer_data_locked(
    std::lock_guard<std::mutex> const&,
    gbm_bo* buffer,
    void const* data,
    size_t count)
{
    if (auto result = gbm_bo_write(buffer, data, count))
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("failed to initialize gbm buffer"))
                << (boost::error_info<Cursor, decltype(result)>(result)));
    }
}

void mgm::Cursor::pad_and_write_image_data_locked(
    std::lock_guard<std::mutex> const& lg,
    GBMBOWrapper& buffer)
{
    auto const orientation = buffer.orientation();
    bool const sideways = orientation == mir_orientation_left || orientation == mir_orientation_right;

    auto const min_width  = sideways ? min_buffer_width : min_buffer_height;
    auto const min_height = sideways ? min_buffer_height : min_buffer_width;

    auto const image_width = std::min(min_width, size.width.as_uint32_t());
    auto const image_height = std::min(min_height, size.height.as_uint32_t());
    auto const image_stride = size.width.as_uint32_t() * 4;

    auto const buffer_stride = std::max(min_width*4, gbm_bo_get_stride(buffer));  // in bytes
    auto const buffer_height = std::max(min_height, gbm_bo_get_height(buffer));
    size_t const padded_size = buffer_stride * buffer_height;

    auto padded = std::unique_ptr<uint8_t[]>(new uint8_t[padded_size]);
    size_t rhs_padding = buffer_stride - 4*image_width;

    auto const filler = 0; // 0x3f; is useful to make buffer visible for debugging
    uint8_t const* src = argb8888.data();
    uint8_t* dest = &padded[0];

    switch (orientation)
    {
    case mir_orientation_normal:
        for (unsigned int y = 0; y < image_height; y++)
        {
            memcpy(dest, src, 4*image_width);
            memset(dest + 4*image_width, filler, rhs_padding);
            dest += buffer_stride;
            src += image_stride;
        }

        memset(dest, 0, buffer_stride * (buffer_height - image_height));
        break;

    case mir_orientation_inverted:
        for (unsigned int row = 0; row != image_height; ++row)
        {
            memset(dest+row*buffer_stride+4*image_width, filler, rhs_padding);

            for (unsigned int col = 0; col != image_width; ++col)
            {
                memcpy(dest+row*buffer_stride+4*col, src + ((image_height-1)-row)*image_stride + 4*((image_width-1)-col), 4);
            }
        }

        memset(dest+image_height*buffer_stride, filler, buffer_stride * (buffer_height - image_height));
        break;

    case mir_orientation_left:
        for (unsigned int row = 0; row != image_width; ++row)
        {
            memset(dest+row*buffer_stride+4*image_height, filler, rhs_padding);

            for (unsigned int col = 0; col != image_height; ++col)
            {
                memcpy(dest+row*buffer_stride+4*col, src + ((image_width-1)-row)*4 + image_stride*col, 4);
            }
        }

        memset(dest+image_width*buffer_stride, filler, buffer_stride * (buffer_height - image_width));
        break;

    case mir_orientation_right:
        for (unsigned int row = 0; row != image_width; ++row)
        {
            memset(dest+row*buffer_stride+4*image_height, filler, rhs_padding);

            for (unsigned int col = 0; col != image_height; ++col)
            {
                memcpy(dest+row*buffer_stride+4*col, src + row*4 + image_stride*((image_height-1)-col), 4);
            }
        }

        memset(dest+image_width*buffer_stride, filler, buffer_stride * (buffer_height - image_width));
        break;
    }

    write_buffer_data_locked(lg, buffer, &padded[0], padded_size);
}

void mgm::Cursor::show(CursorImage const& cursor_image)
{
    std::lock_guard<std::mutex> lg(guard);

    size = cursor_image.size();

    argb8888.resize(size.width.as_uint32_t() * size.height.as_uint32_t() * 4);
    memcpy(argb8888.data(), cursor_image.as_argb_8888(), argb8888.size());

    hotspot = cursor_image.hotspot();
    {
        auto locked_buffers = buffers.lock();
        for (auto& tuple : *locked_buffers)
        {
            pad_and_write_image_data_locked(lg, std::get<2>(tuple));
        }
    }

    // Writing the data could throw an exception so lets
    // hold off on setting visible until after we have succeeded.
    visible = true;
    place_cursor_at_locked(lg, current_position, ForceState);
}

void mgm::Cursor::move_to(geometry::Point position)
{
    place_cursor_at(position, UpdateState);
}

void mir::graphics::mesa::Cursor::suspend()
{
    std::lock_guard<std::mutex> lg(guard);
    clear(lg);
}

void mir::graphics::mesa::Cursor::clear(std::lock_guard<std::mutex> const&)
{
    last_set_failed = false;
    output_container.for_each_output([&](std::shared_ptr<KMSOutput> const& output)
        {
            if (!output->clear_cursor())
                last_set_failed = true;
        });
}

void mgm::Cursor::resume()
{
    place_cursor_at(current_position, ForceState);
}

void mgm::Cursor::hide()
{
    std::lock_guard<std::mutex> lg(guard);
    visible = false;
    clear(lg);
}

void mgm::Cursor::for_each_used_output(
    std::function<void(KMSOutput&, geom::Rectangle const&, MirOrientation orientation)> const& f)
{
    current_configuration->with_current_configuration_do(
        [&f](KMSDisplayConfiguration const& kms_conf)
        {
            kms_conf.for_each_output([&](DisplayConfigurationOutput const& conf_output)
            {
                if (conf_output.used)
                {
                    auto output = kms_conf.get_output_for(conf_output.id);

                    f(*output, conf_output.extents(), conf_output.orientation);
                }
            });
        });
}

void mgm::Cursor::place_cursor_at(
    geometry::Point position,
    ForceCursorState force_state)
{
    std::lock_guard<std::mutex> lg(guard);
    place_cursor_at_locked(lg, position, force_state);
}

void mgm::Cursor::place_cursor_at_locked(
    std::lock_guard<std::mutex> const& lg,
    geometry::Point position,
    ForceCursorState force_state)
{

    current_position = position;

    if (!visible)
        return;

    bool set_on_all_outputs = true;

    for_each_used_output([&](KMSOutput& output, geom::Rectangle const& output_rect, MirOrientation orientation)
    {
        if (output_rect.contains(position))
        {
            auto dp = transform(output_rect, position - output_rect.top_left, orientation);
            auto hs = transform(geom::Rectangle{{0,0}, size}, hotspot, orientation);

            // It's a little strange that we implement hotspot this way as there is
            // drmModeSetCursor2 with hotspot support. However it appears to not actually
            // work on radeon and intel. There also seems to be precedent in weston for
            // implementing hotspot in this fashion.
            output.move_cursor(geom::Point{} + dp - hs);
            auto& buffer = buffer_for_output(output);

            auto const changed_orientation = buffer.change_orientation(orientation);

            if (changed_orientation)
                pad_and_write_image_data_locked(lg, buffer);

            if (force_state || !output.has_cursor() || changed_orientation)
            {
                if (!output.set_cursor(buffer) || !output.has_cursor())
                    set_on_all_outputs = false;
            }
        }
        else
        {
            if (force_state || output.has_cursor())
            {
                output.clear_cursor();
            }
        }
    });

    last_set_failed = !set_on_all_outputs;
}

mgm::Cursor::GBMBOWrapper& mgm::Cursor::buffer_for_output(KMSOutput const& output)
{
    auto const drm_fd = output.drm_fd();
    auto const id = output.id();
    auto locked_buffers = buffers.lock();

    for (auto& bo : *locked_buffers)
    {
        // We use both id and drm_fd as identifier as we're not sure of the uniqueness of either
        if (std::get<0>(bo) == id && std::get<1>(bo) == drm_fd)
            return std::get<2>(bo);
    }

    locked_buffers->push_back(image_buffer{id, drm_fd, GBMBOWrapper{drm_fd, mir_orientation_normal}});

    GBMBOWrapper& bo = std::get<2>(locked_buffers->back());
    if (gbm_bo_get_width(bo) < min_buffer_width)
    {
        min_buffer_width = gbm_bo_get_width(bo);
    }
    if (gbm_bo_get_height(bo) < min_buffer_height)
    {
        min_buffer_height = gbm_bo_get_height(bo);
    }

    return bo;
}
