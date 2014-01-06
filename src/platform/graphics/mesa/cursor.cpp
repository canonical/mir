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

#include <boost/exception/errinfo_errno.hpp>

#include <stdexcept>
#include <vector>

namespace
{
#include "black_arrow.c"
int const width = black_arrow.width;
int const height = black_arrow.height;
}

namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;

mgm::Cursor::GBMBOWrapper::GBMBOWrapper(gbm_device* gbm) :
    buffer(gbm_bo_create(
                gbm,
                width,
                height,
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
    std::shared_ptr<CurrentConfiguration> const& current_configuration) :
        output_container(output_container),
        current_position(),
        buffer(gbm),
        current_configuration(current_configuration)
{
    set_image(black_arrow.pixel_data, geometry::Size{width, height});

    show_at_last_known_position();
}

mgm::Cursor::~Cursor() noexcept
{
    hide();
}

void mgm::Cursor::set_image(const void* raw_argb, geometry::Size size)
{
    if (size != geometry::Size{width, height})
        BOOST_THROW_EXCEPTION(std::logic_error("No support for cursors that aren't 64x64"));

    auto const count = size.width.as_uint32_t() * size.height.as_uint32_t() * sizeof(uint32_t);

    if (auto result = gbm_bo_write(buffer, raw_argb, count))
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("failed to initialize gbm buffer"))
                << (boost::error_info<Cursor, decltype(result)>(result)));
    }
}

void mgm::Cursor::move_to(geometry::Point position)
{
    place_cursor_at(position, UpdateState);
}

void mgm::Cursor::show_at_last_known_position()
{
    place_cursor_at(current_position, ForceState);
}

void mgm::Cursor::hide()
{
    output_container.for_each_output(
        [&](KMSOutput& output) { output.clear_cursor(); });
}

void mgm::Cursor::for_each_used_output(
    std::function<void(KMSOutput&, geom::Rectangle const&)> const& f)
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
                    geom::Rectangle output_rect
                    {
                        conf_output.top_left,
                        conf_output.modes[conf_output.current_mode_index].size
                    };
                    f(*output, output_rect);
                }
            });
        });
}

void mgm::Cursor::place_cursor_at(
    geometry::Point position,
    ForceCursorState force_state)
{
    for_each_used_output([&](KMSOutput& output, geom::Rectangle const& output_rect)
    {
        if (output_rect.contains(position))
        {
            auto dp = position - output_rect.top_left;
            output.move_cursor({dp.dx.as_int(), dp.dy.as_int()});
            if (force_state || !output.has_cursor())
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

    current_position = position;
}
