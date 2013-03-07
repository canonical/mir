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

#ifndef MIR_GRAPHICS_GBM_GBM_DISPLAY_H_
#define MIR_GRAPHICS_GBM_GBM_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "kms_output_container.h"
#include "gbm_display_helpers.h"

#include <vector>

namespace mir
{
namespace geometry
{
class Rectangle;
}
namespace graphics
{

class DisplayReport;

namespace gbm
{

class GBMPlatform;
class KMSOutput;

class GBMDisplay : public Display
{
public:
    GBMDisplay(std::shared_ptr<GBMPlatform> const& platform,
               std::shared_ptr<DisplayReport> const& listener);

    geometry::Rectangle view_area() const;
    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f);

    std::shared_ptr<DisplayConfiguration> configuration();

private:
    void configure(std::shared_ptr<DisplayConfiguration> const& conf);

    std::shared_ptr<GBMPlatform> const platform;
    std::shared_ptr<DisplayReport> const listener;
    helpers::EGLHelper shared_egl;
    std::vector<std::unique_ptr<DisplayBuffer>> display_buffers;
    KMSOutputContainer output_container;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_GBM_DISPLAY_H_ */
