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
#include "client_buffer.h"
#include "mir/thread/all.h"
namespace mcl=mir::client;

static void incRef(android_native_base_t*)
{
}

mcl::MirNativeWindow::MirNativeWindow(ClientSurface* client_surface)
 : surface(client_surface)
{
    ANativeWindow::query = &query_static;
    ANativeWindow::perform = &perform_static;
    ANativeWindow::setSwapInterval = &setSwapInterval_static;
    ANativeWindow::dequeueBuffer = &dequeueBuffer_static;
    ANativeWindow::lockBuffer = &lockBuffer_static;
    ANativeWindow::queueBuffer = &queueBuffer_static;
    ANativeWindow::cancelBuffer = &cancelBuffer_static;

    ANativeWindow::common.incRef = &incRef;
    ANativeWindow::common.decRef = &incRef;

    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;

    printf("PIDcreat %i\n", gettid()); 
}

int mcl::MirNativeWindow::convert_pixel_format(MirPixelFormat mir_pixel_format) const
{
    switch(mir_pixel_format)
    {
        case mir_pixel_format_rgba_8888:
//            return HAL_PIXEL_FORMAT_RGBA_8888;
            return 5; 
        default:
            return 0;
    }
}

#include <iostream> 
int mcl::MirNativeWindow::dequeueBuffer (struct ANativeWindowBuffer** buffer_to_driver)
{
    int ret = -1000;
    auto buffer = surface->get_current_buffer();
    *buffer_to_driver = buffer->get_native_handle();
    return ret;
}

int mcl::MirNativeWindow::query(int key, int* value ) const
{
    int ret = 0;
    switch (key)
    {
        case NATIVE_WINDOW_WIDTH:
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            *value = surface->get_parameters().width;
            break;
        case NATIVE_WINDOW_HEIGHT:
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            *value = surface->get_parameters().height;
            break;
        case NATIVE_WINDOW_FORMAT:
            *value = convert_pixel_format(surface->get_parameters().pixel_format);
            break;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0; /* transform hint is a bitmask. 0 means no transform */
            break;
        default:
            printf("ERROR\n");
            ret = -1;
            break;
    }
    printf("VALUE %i\n", *value);
    return ret;
}

int mcl::MirNativeWindow::query_static(const ANativeWindow* anw, int key, int* value)
{
    printf("QUERY: %i\n", key);
    auto self = static_cast<const mcl::MirNativeWindow*>(anw);
    return self->query(key, value);
} 

int mcl::MirNativeWindow::perform_static(ANativeWindow*, int key, ...)
{
    va_list vl;
    va_start(vl,key);
    /* todo: kdub: the driver will send us requests sometimes via this hook. we will 
                   probably have to service these requests eventually */
    
    auto arg1 = va_arg(vl, int);
    printf("perform! %i %i\n", key, arg1);
    va_end(vl);
    return 0;
} 

int mcl::MirNativeWindow::setSwapInterval_static (struct ANativeWindow* window, int interval)
{
    printf("int is %i\n", window->maxSwapInterval);
    printf("setswapinterval! %i\n", interval);
    return 0;
}

int mcl::MirNativeWindow::dequeueBuffer_static (struct ANativeWindow* window, struct ANativeWindowBuffer** buffer)
{
    auto self = static_cast<const mcl::MirNativeWindow*>(window);
    return self->dequeueBuffer(buffer);
}

int mcl::MirNativeWindow::lockBuffer_static(struct ANativeWindow* /*window*/, struct ANativeWindowBuffer* /*buffer*/)
{
    printf("lock!\n");
    return 0;
}

int mcl::MirNativeWindow::queueBuffer_static(struct ANativeWindow* /*window*/, struct ANativeWindowBuffer* /*buffer*/)
{
    printf("queue!\n");
    return 0;
}

int mcl::MirNativeWindow::cancelBuffer_static(struct ANativeWindow* /*window*/, struct ANativeWindowBuffer* /*buffer*/)
{
    printf("cancel!\n");
    return 0;
}
