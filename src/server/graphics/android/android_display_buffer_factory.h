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

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_BUFFER_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_BUFFER_FACTORY_H_

#include <memory>

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{

class DisplayBuffer;

namespace android
{
class DisplayDevice;

class AndroidDisplayBufferFactory
{
public:
    virtual ~AndroidDisplayBufferFactory() = default;

    virtual std::unique_ptr<DisplayBuffer> create_display_buffer(
        std::shared_ptr<DisplayDevice> const& display_device) = 0;
    virtual std::shared_ptr<DisplayDevice> create_display_device() = 0;
    virtual EGLDisplay egl_display() = 0;
    virtual EGLConfig egl_config() = 0;
    virtual EGLContext shared_egl_context() = 0;

protected:
    AndroidDisplayBufferFactory() = default;
    AndroidDisplayBufferFactory(AndroidDisplayBufferFactory const&) = delete;
    AndroidDisplayBufferFactory& operator=(AndroidDisplayBufferFactory const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_BUFFER_FACTORY_H_ */
