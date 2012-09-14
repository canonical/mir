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
#include "gbm_display_helpers.h"

namespace mir
{
namespace geometry
{
class Rectangle;
}
namespace graphics
{
namespace gbm
{

class BufferObject;

class GBMDisplay : public Display
{
public:
    GBMDisplay();
    ~GBMDisplay();

    geometry::Rectangle view_area() const;
    bool post_update();

private:
    BufferObject* get_front_buffer_object();
    bool schedule_and_wait_for_page_flip(BufferObject* bufobj);

    BufferObject* last_flipped_bufobj;
    /* Order is important for construction/destruction */
    helpers::DRMHelper drm;
    helpers::KMSHelper kms;
    helpers::GBMHelper gbm;
    helpers::EGLHelper egl;
};

}
}
}
#endif /* MIR_GRAPHICS_GBM_GBM_DISPLAY_H_ */
