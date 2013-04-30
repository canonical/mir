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

namespace mir
{

namespace compositor
{
class BufferSwapper;
}

namespace graphics
{
namespace android
{

class InternalClientWindow : public AndroidDriverInterpreter
{
public:
    InternalClientWindow(std::unique_ptr<compositor::BufferSwapper>&&)
    {
    }

    ANativeWindowBuffer* driver_requests_buffer()
    {
        return nullptr;
    }
    void driver_returns_buffer(ANativeWindowBuffer*, std::shared_ptr<SyncObject> const&)
    {}

    virtual void dispatch_driver_request_format(int)
    {}

    virtual int  driver_requests_info(int) const
    {return 8;}
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_ */
