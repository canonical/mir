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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_MIR_NATIVE_WINDOW_H_
#define MIR_CLIENT_ANDROID_MIR_NATIVE_WINDOW_H_

#include "mir_client_surface.h"
#include <system/window.h>

namespace mir
{
namespace client
{

class MirNativeWindow : public ANativeWindow
{
public:
    explicit MirNativeWindow(ClientSurface* surface);

private:
    static int query_static(const ANativeWindow* anw, int key, int* value);
    static int perform_static(ANativeWindow* anw, int key, ...);

    /* anw functions */
    int query(int key, int* value) const;


    /* helpers */
    int convert_pixel_format(MirPixelFormat mir_pixel_format) const;

    /* todo: we should probably take ownership of the ClientSurface */
    ClientSurface * surface; 
};

}
}

#endif /* MIR_CLIENT_CLIENT_SURFACE_H_ */
