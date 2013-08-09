/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_
#define MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_

#include "mir/graphics/android/android_driver_interpreter.h"
#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"

namespace mir
{
namespace graphics
{
class InternalSurface;

namespace android
{
class InterpreterResourceCache;
class InternalClientWindow : public AndroidDriverInterpreter
{
public:
    InternalClientWindow(std::shared_ptr<InternalSurface> const&,
                         std::shared_ptr<InterpreterResourceCache> const&);
    ANativeWindowBuffer* driver_requests_buffer();
    void driver_returns_buffer(ANativeWindowBuffer*, std::shared_ptr<SyncObject> const&);
    void dispatch_driver_request_format(int);
    int  driver_requests_info(int) const;
    void sync_to_display(bool sync); 

private:
    std::shared_ptr<InternalSurface> const surface;
    std::shared_ptr<InterpreterResourceCache> const resource_cache;
    int format;
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_ */
