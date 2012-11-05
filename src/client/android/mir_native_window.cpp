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

#include "mir_client/android/mir_native_window.h"
#include "mir_client/client_buffer.h"
#include "mir/thread/all.h"
namespace mcl=mir::client;

static void incRef(android_native_base_t*)
{
}

mcl::MirNativeWindow::MirNativeWindow(ClientSurface* client_surface)
 : surface(client_surface),
   driver_pixel_format(-1)
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
}

int mcl::MirNativeWindow::dequeueBuffer (struct ANativeWindowBuffer** buffer_to_driver)
{
    int ret = 0;
    auto buffer = surface->get_current_buffer();
    *buffer_to_driver = buffer->get_native_handle();
    (*buffer_to_driver)->format = driver_pixel_format;
    return ret;
}

static void empty(MirSurface * /*surface*/, void * /*client_context*/)
{}
int mcl::MirNativeWindow::queueBuffer(struct ANativeWindowBuffer* /*buffer*/)
{
    mir_wait_for(surface->next_buffer(empty, NULL));
    return 0;
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
            *value = driver_pixel_format;
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

int mcl::MirNativeWindow::perform(int key, va_list arg_list )
{
    int ret = 0;
    va_list args;
    va_copy(args, arg_list);

    switch(key)
    {
        case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
            driver_pixel_format = va_arg(args, int); 
            break;
        default:
            break;
    }

    va_end(args);
    return ret;
}

int mcl::MirNativeWindow::perform_static(ANativeWindow* window, int key, ...)
{
    va_list args;
    va_start(args, key);
    auto self = static_cast<const mcl::MirNativeWindow*>(window);
    auto ret = self->perform(key, args);
    va_end(args);

    return ret;
} 


int mcl::MirNativeWindow::dequeueBuffer_static (struct ANativeWindow* window, struct ANativeWindowBuffer** buffer)
{   
    auto self = static_cast<const mcl::MirNativeWindow*>(window);
    return self->dequeueBuffer(buffer);
}

int mcl::MirNativeWindow::queueBuffer_static(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer)
{
    auto self = static_cast<const mcl::MirNativeWindow*>(window);
    return self->queueBuffer(buffer);
}

/* setSwapInterval, lockBuffer, and cancelBuffer don't seem to being called by the driver. for now just return without calling into MirNativeWindow */
int mcl::MirNativeWindow::setSwapInterval_static (struct ANativeWindow* /*window*/, int /*interval*/)
{
    return 0;
}

int mcl::MirNativeWindow::lockBuffer_static(struct ANativeWindow* /*window*/, struct ANativeWindowBuffer* /*buffer*/)
{
    return 0;
}

int mcl::MirNativeWindow::cancelBuffer_static(struct ANativeWindow* /*window*/, struct ANativeWindowBuffer* /*buffer*/)
{
    return 0;
}
