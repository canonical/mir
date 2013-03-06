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

#include "anativewindow_interpreter.h"

namespace mcla=mir::client::android;

mcla::ANativeWindowInterpreter::ANativeWindowInterpreter(ClientSurface* surface)
 :surface(surface)
{
}

ANativeWindowBuffer* mcla::ANativeWindowInterpreter::driver_requests_buffer()
{
    return 0x0;
}

void mcla::ANativeWindowInterpreter::driver_returns_buffer(ANativeWindowBuffer*, int /*fence_fd*/ )
{
}

void mcla::ANativeWindowInterpreter::dispatch_driver_request_format(int /*format*/)
{
}

int  mcla::ANativeWindowInterpreter::driver_requests_info(int /* key */) const
{
    return 0;
}

#if 0
int mcla::ANativeWindowInterpreter::dequeueBuffer_internal (struct ANativeWindowBuffer** /*buffer_to_driver*/)
{
    int ret = 0;
    auto buffer = surface->get_current_buffer();
    *buffer_to_driver = buffer->get_native_handle();
    (*buffer_to_driver)->format = driver_pixel_format;
    return ret;
}

static void empty(MirSurface * /*surface*/, void * /*client_context*/)
{}
int mcla::ANativeWindowInterpreter::queueBuffer_internal(struct ANativeWindowBuffer* /*buffer*/)
{
    mir_wait_for(surface->next_buffer(empty, NULL));
    return 0;
}


int mcla::ANativeWindowInterpreter::perform_internal(int key, va_list arg_list )
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

int mcla::ANativeWindowInterpreter::query_internal(int /*key*/, int* /*value*/ ) const
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
#endif
