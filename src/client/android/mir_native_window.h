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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_MIR_NATIVE_WINDOW_H_
#define MIR_CLIENT_ANDROID_MIR_NATIVE_WINDOW_H_

#include "../mir_client_surface.h"
#include <system/window.h>
#include <cstdarg>

namespace mir
{
namespace client
{
namespace android
{

class MirNativeWindow : public ANativeWindow
{
public:
    explicit MirNativeWindow(ClientSurface* surface);

    int query(int key, int* value) const;
    int perform(int key, va_list args );
    int dequeueBuffer(struct ANativeWindowBuffer** buffer);
    int queueBuffer(struct ANativeWindowBuffer* buffer);
private:

    ClientSurface * surface;
    int driver_pixel_format;
};

}
}
}

#endif /* MIR_CLIENT_ANDROID_MIR_NATIVE_WINDOW_H_ */
