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

#ifndef MIR_GRAPHICS_GBM_GBM_DISPLAY_H_
#define MIR_GRAPHICS_GBM_GBM_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/display_listener.h"
#include "mir/graphics/platform.h"
#include "gbm_display_helpers.h"

namespace mir
{
namespace geometry
{
class Rectangle;
}
namespace logging
{
class Logger;
}

namespace graphics
{
namespace gbm
{

class GBMPlatform;
class BufferObject;

class GBMDisplayReporter;

class GBMDisplay : public Display
{
public:
    GBMDisplay(const std::shared_ptr<GBMPlatform>& platform, const std::shared_ptr<DisplayListener>& reporter);

    ~GBMDisplay();

    geometry::Rectangle view_area() const;
    void clear();
    bool post_update();

    std::shared_ptr<DisplayConfiguration> configuration();

private:
    BufferObject* get_front_buffer_object();
    bool schedule_and_wait_for_page_flip(BufferObject* bufobj);

    BufferObject* last_flipped_bufobj;
    std::shared_ptr<GBMPlatform> platform;
    std::shared_ptr<DisplayListener> listener;
    /* DRM and GBM helpers from GBMPlatform */
    helpers::DRMHelper& drm;
    helpers::GBMHelper& gbm;
    /* Order is important for construction/destruction */
    helpers::KMSHelper  kms;
    helpers::EGLHelper  egl;
};

}
}
}
#endif /* MIR_GRAPHICS_GBM_GBM_DISPLAY_H_ */
