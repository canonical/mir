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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_CLIENT_SURFACE_INTERPRETER_H_
#define MIR_CLIENT_ANDROID_CLIENT_SURFACE_INTERPRETER_H_

#include "mir/graphics/android/android_driver_interpreter.h"
#include "../mir_client_surface.h"

namespace mir
{
namespace graphics
{
namespace android
{
class SyncFileOps;
}
}
namespace client
{
namespace android
{

class ClientSurfaceInterpreter : public graphics::android::AndroidDriverInterpreter
{
public:
    explicit ClientSurfaceInterpreter(ClientSurface& surface);

    graphics::NativeBuffer* driver_requests_buffer();
    void driver_returns_buffer(ANativeWindowBuffer*, int fence_fd );
    void dispatch_driver_request_format(int format);
    int  driver_requests_info(int key) const;
    void sync_to_display(bool);

private:
    ClientSurface& surface;
    int driver_pixel_format;
    std::shared_ptr<graphics::android::SyncFileOps> const sync_ops;
};

}
}
}

#endif /* MIR_CLIENT_ANDROID_CLIENT_SURFACE_INTERPRETER_H_ */
