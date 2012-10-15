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
    ANativeWindow::perform = &perform_static;
}

int mcl::MirNativeWindow::convert_pixel_format(MirPixelFormat mir_pixel_format) const
{
    switch(mir_pixel_format)
    {
        case mir_pixel_format_rgba_8888:
            return HAL_PIXEL_FORMAT_RGBA_8888; 
        default:
            return 0;
    }
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
        case NATIVE_WINDOW_FORMAT:
            *value = convert_pixel_format(surface->get_parameters().pixel_format);
            break;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0; /* transform hint is a bitmask. 0 means no transform */
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

int mcl::MirNativeWindow::perform_static(ANativeWindow*, int, ...)
{
    /* todo: kdub: the driver will send us requests sometimes via this hook. we will 
                   probably have to service these requests eventually */
    return 0;
} 
