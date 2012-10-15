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

#include "android/mir_native_window.h"

namespace mcl=mir::client;

mcl::MirNativeWindow::MirNativeWindow(ClientSurface* client_surface)
 : surface(client_surface)
{
    ANativeWindow::query = &query_static;
}

int mcl::MirNativeWindow::query(int key, int* value ) const
{
    int ret = 0;
    switch (key)
    {
        case NATIVE_WINDOW_WIDTH:
            *value = surface->get_parameters().width;
            break;
        case NATIVE_WINDOW_HEIGHT:
            *value = surface->get_parameters().height;
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}
 
int mcl::MirNativeWindow::query_static(const ANativeWindow* anw, int key, int* value)
{
    auto self = static_cast<const mcl::MirNativeWindow*>(anw);
    return self->query(key, value);
} 

