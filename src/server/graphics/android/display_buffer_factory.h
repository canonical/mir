/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_BUFFER_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_BUFFER_FACTORY_H_

#include "android_display_buffer_factory.h"
#include "hardware/hwcomposer.h"
#include "hardware/fb.h"

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{
class DisplayResourceFactory;

class DisplayBufferFactory : public AndroidDisplayBufferFactory
{
public:
    DisplayBufferFactory(
        std::shared_ptr<DisplayResourceFactory> const& res_factory,
        std::shared_ptr<DisplayReport> const& display_report,
        bool should_use_fb_fallback);

    std::shared_ptr<DisplayDevice> create_display_device();
    std::unique_ptr<DisplayBuffer> create_display_buffer(
        std::shared_ptr<DisplayDevice> const& display_device,
        GLContext const& gl_context);

private:
    std::shared_ptr<DisplayResourceFactory> const res_factory;
    std::shared_ptr<DisplayReport> const display_report;
    bool force_backup_display;

    std::shared_ptr<hwc_composer_device_1> hwc_native;
    std::shared_ptr<framebuffer_device_t> fb_native;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_BUFFER_FACTORY_H_ */
