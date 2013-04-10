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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_SERVER_RENDER_WINDOW_H_
#define MIR_GRAPHICS_ANDROID_SERVER_RENDER_WINDOW_H_

#include "mir/graphics/android/android_driver_interpreter.h"
#include "mir/compositor/buffer_id.h"

#include <hardware/fb.h>
#include <hardware/hwcomposer.h>

#include <memory>
#include <unordered_map>

namespace mir
{

namespace compositor
{
class BufferSwapper;
class Buffer;
class NativeBufferHandle;
}

namespace graphics
{
namespace android
{

class DisplayInfoProvider;
class ServerRenderWindow : public client::android::AndroidDriverInterpreter 
{
public:
    ServerRenderWindow(std::shared_ptr<compositor::BufferSwapper> const& swapper,
                       std::shared_ptr<DisplayInfoProvider> const& display_poster);

    ANativeWindowBuffer* driver_requests_buffer();
    void driver_returns_buffer(ANativeWindowBuffer*);
    void dispatch_driver_request_format(int format);
    int driver_requests_info(int key) const;

private:
    std::shared_ptr<compositor::BufferSwapper> swapper;
    std::shared_ptr<DisplayInfoProvider> poster;
    unsigned int native_format;

    std::unordered_map<ANativeWindowBuffer*, std::shared_ptr<compositor::Buffer>> buffers_in_driver; 
}; 

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_SERVER_RENDER_WINDOW_H_ */
