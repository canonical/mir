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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_GRAPHIC_GBM_DISPLAY_REPORTER_H_
#define MIR_GRAPHIC_GBM_DISPLAY_REPORTER_H_

#include "mir/graphics/display_listener.h"

#include <memory>

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

class GBMDisplayReporter : public DisplayListener
{
  public:

    static const char* component();

    GBMDisplayReporter(const std::shared_ptr<logging::Logger>& logger);
    virtual ~GBMDisplayReporter();

    virtual void report_successful_setup_of_native_resources();
    virtual void report_successful_egl_make_current_on_construction();
    virtual void report_successful_egl_buffer_swap_on_construction();
    virtual void report_successful_drm_mode_set_crtc_on_construction();
    virtual void report_successful_display_construction();
    virtual void report_page_flip_timeout();

  protected:
    GBMDisplayReporter(const GBMDisplayReporter&) = delete;
    GBMDisplayReporter& operator=(const GBMDisplayReporter&) = delete;

  private:
    std::shared_ptr<logging::Logger> logger;
};
}
}
}

#endif /* MIR_GRAPHIC_GBM_DISPLAY_REPORTER_H_ */
