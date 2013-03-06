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

#include "mir_native_window.h"
#include "../client_buffer.h"
#include "android_driver_interpreter.h"

namespace mcl=mir::client;
namespace mcla=mir::client::android;

namespace
{
static int query_static(const ANativeWindow* anw, int key, int* value);
static int perform_static(ANativeWindow* anw, int key, ...);
static int setSwapInterval_static (struct ANativeWindow* window, int interval);
static int dequeueBuffer_static (struct ANativeWindow* window,
                                 struct ANativeWindowBuffer** buffer);
static int lockBuffer_static(struct ANativeWindow* window,
                             struct ANativeWindowBuffer* buffer);
static int queueBuffer_static(struct ANativeWindow* window,
                              struct ANativeWindowBuffer* buffer);
static int cancelBuffer_static(struct ANativeWindow* window,
                               struct ANativeWindowBuffer* buffer);

static void incRef(android_native_base_t*)
{
}

int query_static(const ANativeWindow* anw, int key, int* value)
{
    auto self = static_cast<const mcla::MirNativeWindow*>(anw);
    return self->query_internal(key, value);
}

int perform_static(ANativeWindow* window, int key, ...)
{
    va_list args;
    va_start(args, key);
    auto self = static_cast<mcla::MirNativeWindow*>(window);
    auto ret = self->perform_internal(key, args);
    va_end(args);

    return ret;
}

int dequeueBuffer_static (struct ANativeWindow* window,
                          struct ANativeWindowBuffer** buffer)
{
    auto self = static_cast<mcla::MirNativeWindow*>(window);
    return self->dequeueBuffer_internal(buffer);
}

int queueBuffer_static(struct ANativeWindow* window,
                       struct ANativeWindowBuffer* buffer)
{
    auto self = static_cast<mcla::MirNativeWindow*>(window);
    return self->queueBuffer_internal(buffer);
}

/* setSwapInterval, lockBuffer, and cancelBuffer don't seem to being called by the driver. for now just return without calling into MirNativeWindow */
int setSwapInterval_static (struct ANativeWindow* /*window*/, int /*interval*/)
{
    return 0;
}

int lockBuffer_static(struct ANativeWindow* /*window*/,
                      struct ANativeWindowBuffer* /*buffer*/)
{
    return 0;
}

int cancelBuffer_static(struct ANativeWindow* /*window*/,
                        struct ANativeWindowBuffer* /*buffer*/)
{
    return 0;
}

}

mcla::MirNativeWindow::MirNativeWindow(std::shared_ptr<AndroidDriverInterpreter> interpreter)
 : driver_interpreter(interpreter),
   driver_pixel_format(-1)
{
    ANativeWindow::query = &query_static;
    ANativeWindow::perform = &perform_static;
    ANativeWindow::setSwapInterval = &setSwapInterval_static;
    ANativeWindow::dequeueBuffer_DEPRECATED = &dequeueBuffer_static;
    ANativeWindow::lockBuffer_DEPRECATED = &lockBuffer_static;
    ANativeWindow::queueBuffer_DEPRECATED = &queueBuffer_static;
    ANativeWindow::cancelBuffer_DEPRECATED = &cancelBuffer_static;

    ANativeWindow::common.incRef = &incRef;
    ANativeWindow::common.decRef = &incRef;

    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;
}

int mcla::MirNativeWindow::dequeueBuffer_internal (struct ANativeWindowBuffer** /*buffer_to_driver*/)
{
    int ret = 0;
#if 0
    auto buffer = surface->get_current_buffer();
    *buffer_to_driver = buffer->get_native_handle();
    (*buffer_to_driver)->format = driver_pixel_format;
#endif
    return ret;
}

#if 0
static void empty(MirSurface * /*surface*/, void * /*client_context*/)
{}
#endif
int mcla::MirNativeWindow::queueBuffer_internal(struct ANativeWindowBuffer* /*buffer*/)
{
#if 0
    mir_wait_for(surface->next_buffer(empty, NULL));
#endif
    return 0;
}

int mcla::MirNativeWindow::query_internal(int key, int* value ) const
{
    *value = driver_interpreter->driver_requests_info(key);
    return 0;
}

int mcla::MirNativeWindow::perform_internal(int key, va_list arg_list )
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

