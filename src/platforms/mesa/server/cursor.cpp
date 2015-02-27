/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
#include <drm/drm.h>

#include <boost/exception/errinfo_errno.hpp>

#include <stdexcept>
#include <vector>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace geom = mir::geometry;

namespace
{
const uint64_t fallback_cursor_size = 64;

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
   uint64_t height;
   if (drmGetCap(fd, DRM_CAP_CURSOR_HEIGHT, &height) < 0)
       height = fallback_cursor_size;
   return int(height);
}

int get_drm_cursor_width(int fd)
{
   uint64_t width;
   if (drmGetCap(fd, DRM_CAP_CURSOR_WIDTH, &width) < 0)
       width = fallback_cursor_size;
   return int(width);
}
}

mgm::Cursor::GBMBOWrapper::GBMBOWrapper(gbm_device* gbm) :
    buffer(gbm_bo_create(
                gbm,
                get_drm_cursor_width(gbm_device_get_fd(gbm)),
                get_drm_cursor_height(gbm_device_get_fd(gbm)),
                GBM_FORMAT_ARGB8888,
                GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE))
{
    if (!buffer) BOOST_THROW_EXCEPTION(std::runtime_error("failed to create gbm buffer"));
}

inline mgm::Cursor::GBMBOWrapper::operator gbm_bo*() { return buffer; }
inline mgm::Cursor::GBMBOWrapper::~GBMBOWrapper()    { gbm_bo_destroy(buffer); }

mgm::Cursor::Cursor(
    gbm_device* gbm,
    KMSOutputContainer& output_container,
    std::shared_ptr<CurrentConfiguration> const& current_configuration,
    std::shared_ptr<mg::CursorImage> const& initial_image) :
        output_container(output_container),
        current_position(),
        visible(true),
        buffer(gbm),
        buffer_width(gbm_bo_get_width(buffer)),
        buffer_height(gbm_bo_get_height(buffer)),
        current_configuration(current_configuration)
{
    show(*initial_image);
}

mgm::Cursor::~Cursor() noexcept
{
    hide();
}

void mgm::Cursor::write_buffer_data_locked(std::lock_guard<std::mutex> const&, void const* data, size_t count)
{
    if (auto result = gbm_bo_write(buffer, data, count))
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("failed to initialize gbm buffer"))
                << (boost::error_info<Cursor, decltype(result)>(result)));
    }
}

void mgm::Cursor::pad_and_write_image_data_locked(std::lock_guard<std::mutex> const& lg, CursorImage const& image)
{
    auto image_argb = static_cast<uint8_t const*>(image.as_argb_8888());
    auto image_width = image.size().width.as_uint32_t();
    auto image_height = image.size().height.as_uint32_t();
    auto image_stride = image_width * 4;

    if (image_width > buffer_width || image_height > buffer_height)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Image is too big for GBM cursor buffer"));
    }
    
    size_t buffer_stride = gbm_bo_get_stride(buffer);  // in bytes
    size_t padded_size = buffer_stride * buffer_height;
    auto padded = std::unique_ptr<uint8_t[]>(new uint8_t[padded_size]);
    size_t rhs_padding = buffer_stride - image_stride;

    uint8_t* dest = &padded[0];
    uint8_t const* src = image_argb;

    for (unsigned int y = 0; y < image_height; y++)
    {
        memcpy(dest, src, image_stride);
        memset(dest + image_stride, 0, rhs_padding);
        dest += buffer_stride;
        src += image_stride;
    }

    memset(dest, 0, buffer_stride * (buffer_height - image_height));

    write_buffer_data_locked(lg, &padded[0], padded_size);
}

void mgm::Cursor::show()
{
    if (!visible)
    {
        std::lock_guard<std::mutex> lg(guard);
        visible = true;
        place_cursor_at_locked(lg, current_position, ForceState);
    }
}

void mgm::Cursor::show(CursorImage const& cursor_image)
{
    std::lock_guard<std::mutex> lg(guard);

    auto const& size = cursor_image.size();

    if (size != geometry::Size{buffer_width, buffer_height})
    {
        pad_and_write_image_data_locked(lg, cursor_image);
    }
    else
    {
        auto const count = size.width.as_uint32_t() * size.height.as_uint32_t() * sizeof(uint32_t);
        write_buffer_data_locked(lg, cursor_image.as_argb_8888(), count);
    }
    hotspot = cursor_image.hotspot();
    
    // The hotspot may have changed so we need to call drmModeSetCursor again if the cursor was already visible.
    if (visible)
        place_cursor_at_locked(lg, current_position, ForceState);

    // Writing the data could throw an exception so lets
    // hold off on setting visible until after we have succeeded.
    visible = true;
}

void mgm::Cursor::move_to(geometry::Point position)
{
    place_cursor_at(position, UpdateState);
}

void mir::graphics::mesa::Cursor::suspend()
{
    std::lock_guard<std::mutex> lg(guard);

    output_container.for_each_output(
        [&](KMSOutput& output) { output.clear_cursor(); });
}

void mgm::Cursor::resume()
{
    place_cursor_at(current_position, ForceState);
}

void mgm::Cursor::hide()
{
    std::lock_guard<std::mutex> lg(guard);
    visible = false;

    output_container.for_each_output(
        [&](KMSOutput& output) { output.clear_cursor(); });
}

void mgm::Cursor::for_each_used_output(
    std::function<void(KMSOutput&, geom::Rectangle const&, MirOrientation orientation)> const& f)
{
    current_configuration->with_current_configuration_do(
        [this,&f](KMSDisplayConfiguration const& kms_conf)
        {
            kms_conf.for_each_output([&](DisplayConfigurationOutput const& conf_output)
            {
                if (conf_output.used)
                {
                    uint32_t const connector_id = kms_conf.get_kms_connector_id(conf_output.id);
                    auto output = output_container.get_kms_output_for(connector_id);

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
    std::lock_guard<std::mutex> const&,
    geometry::Point position,
    ForceCursorState force_state)
{

    current_position = position;

    if (!visible)
        return;

    for_each_used_output([&](KMSOutput& output, geom::Rectangle const& output_rect, MirOrientation orientation)
    {
        if (output_rect.contains(position))
        {
            auto dp = transform(output_rect, position - output_rect.top_left, orientation);
            
            // It's a little strange that we implement hotspot this way as there is
            // drmModeSetCursor2 with hotspot support. However it appears to not actually
            // work on radeon and intel. There also seems to be precedent in weston for
            // implementing hotspot in this fashion.
            output.move_cursor(geom::Point{} + dp - hotspot);
            if (force_state || !output.has_cursor()) // TODO - or if orientation had changed - then set buffer..
            {
                output.set_cursor(buffer);
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
}
