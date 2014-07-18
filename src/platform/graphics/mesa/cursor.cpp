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

#include <boost/exception/errinfo_errno.hpp>

#include <stdexcept>
#include <vector>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace geom = mir::geometry;

namespace
{
int const buffer_width = 64;
int const buffer_height = 64;

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
}

mgm::Cursor::GBMBOWrapper::GBMBOWrapper(gbm_device* gbm) :
    buffer(gbm_bo_create(
                gbm,
                buffer_width,
                buffer_height,
                GBM_FORMAT_ARGB8888,
                GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE))
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
    uint32_t const* image_argb = static_cast<uint32_t const*>(image.as_argb_8888());
    auto image_width = image.size().width.as_uint32_t();
    auto image_height = image.size().height.as_uint32_t();

    if (image_width > buffer_width || image_height > buffer_height)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Image is too big for GBM cursor buffer"));
    }
    
    uint32_t pixels[buffer_width*buffer_height] {};
    // 'pixels' is initialized to transparent so we just need to copy the initial image
    //  in to the top left corner.
    for (unsigned int i = 0; i < image_height; i++)
    {
        for (unsigned int j = 0; j < image_width; j++)
        {
            pixels[buffer_width*i+j] = image_argb[image_width*i + j];
        }
    }

    auto const count = buffer_width * buffer_height * sizeof(uint32_t);
    write_buffer_data_locked(lg, pixels, count);
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
            output.move_cursor(geom::Point{dp.dx.as_int(), dp.dy.as_int()} - hotspot);
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
