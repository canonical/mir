/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_display.h"
#include "gbm_platform.h"
#include "gbm_display_buffer.h"
#include "kms_display_configuration.h"
#include "kms_output.h"
#include "kms_page_flipper.h"

#include "mir/geometry/rectangle.h"

namespace mgg = mir::graphics::gbm;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mgg::GBMDisplay::GBMDisplay(std::shared_ptr<GBMPlatform> const& platform,
                            std::shared_ptr<DisplayReport> const& listener)
    : platform(platform),
      listener(listener),
      output_container{platform->drm.fd,
                       std::make_shared<KMSPageFlipper>(platform->drm.fd)}
{
    shared_egl.setup(platform->gbm);

    configure(configuration());

    shared_egl.make_current();
}

geom::Rectangle mgg::GBMDisplay::view_area() const
{
    return display_buffers[0]->view_area();
}

void mgg::GBMDisplay::for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f)
{
    for (auto& db_ptr : display_buffers)
        f(*db_ptr);
}

std::shared_ptr<mg::DisplayConfiguration> mgg::GBMDisplay::configuration()
{
    return std::make_shared<mgg::KMSDisplayConfiguration>(platform->drm.fd);
}

void mgg::GBMDisplay::configure(std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    display_buffers.clear();
    std::vector<std::shared_ptr<KMSOutput>> enabled_outputs;

    /* Create or reset the KMS outputs */
    conf->for_each_output([&](DisplayConfigurationOutput const& conf_output)
    {
        auto const& kms_conf = std::static_pointer_cast<KMSDisplayConfiguration>(conf);
        uint32_t const connector_id = kms_conf->get_kms_connector_id(conf_output.id);

        auto output = output_container.get_kms_output_for(connector_id);

        if (conf_output.connected)
            enabled_outputs.push_back(output);
    });

    geom::Size max_size;

    /* Find the size of the largest enabled output... */
    for (auto const& output : enabled_outputs)
    {
        if (output->size().width > max_size.width)
            max_size.width = output->size().width;
        if (output->size().height > max_size.height)
            max_size.height = output->size().height;
    }

    /* ...and create a scanout surface with that size */
    auto surface = platform->gbm.create_scanout_surface(max_size.width.as_uint32_t(),
                                                        max_size.height.as_uint32_t());

    /* Create a single DisplayBuffer that displays the surface on all the outputs */
    std::unique_ptr<DisplayBuffer> db{new GBMDisplayBuffer{platform, listener, enabled_outputs,
                                                           std::move(surface), max_size,
                                                           shared_egl.context()}};
    display_buffers.push_back(std::move(db));
}
