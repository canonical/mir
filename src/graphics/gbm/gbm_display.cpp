/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_display.h"
#include "gbm_platform.h"
#include "gbm_display_buffer.h"
#include "kms_display_configuration.h"

#include "mir/geometry/rectangle.h"

namespace mgg = mir::graphics::gbm;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mgg::GBMDisplay::GBMDisplay(std::shared_ptr<GBMPlatform> const& platform,
                            std::shared_ptr<DisplayListener> const& listener)
    : platform(platform)
{
    std::unique_ptr<DisplayBuffer> db{new GBMDisplayBuffer{platform, listener}};
    display_buffers.push_back(std::move(db));
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
