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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/mir_native_window.h"
#include "mir/graphics/android/android_driver_interpreter.h"
#include "mir/graphics/android/sync_fence.h"

#include "mir/uncaught.h"

#include <iostream>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

namespace
{

static int query_static(const ANativeWindow* anw, int key, int* value);
static int perform_static(ANativeWindow* anw, int key, ...);
static int setSwapInterval_static (struct ANativeWindow* window, int interval);
static int dequeueBuffer_deprecated_static (struct ANativeWindow* window,
                                 struct ANativeWindowBuffer** buffer);
static int dequeueBuffer_static (struct ANativeWindow* window,
                                 struct ANativeWindowBuffer** buffer, int* fence_fd);
static int lockBuffer_static(struct ANativeWindow* window,
                             struct ANativeWindowBuffer* buffer);
static int queueBuffer_deprecated_static(struct ANativeWindow* window,
                              struct ANativeWindowBuffer* buffer);
static int queueBuffer_static(struct ANativeWindow* window,
                              struct ANativeWindowBuffer* buffer, int fence_fd);
static int cancelBuffer_static(struct ANativeWindow* window,
                               struct ANativeWindowBuffer* buffer, int fence_fd);
static int cancelBuffer_deprecated_static(struct ANativeWindow* window,
                               struct ANativeWindowBuffer* buffer);

static void incRef(android_native_base_t*)
{
}

int query_static(const ANativeWindow* anw, int key, int* value)
{
    auto self = static_cast<const mga::MirNativeWindow*>(anw);
    return self->query(key, value);
}

int perform_static(ANativeWindow* window, int key, ...)
{
    va_list args;
    va_start(args, key);
    auto self = static_cast<mga::MirNativeWindow*>(window);
    auto ret = self->perform(key, args);
    va_end(args);

    return ret;
}

int dequeueBuffer_deprecated_static (struct ANativeWindow* window,
                          struct ANativeWindowBuffer** buffer)
{
    auto self = static_cast<mga::MirNativeWindow*>(window);
    return self->dequeueBufferAndWait(buffer);
}

int dequeueBuffer_static (struct ANativeWindow* window,
                          struct ANativeWindowBuffer** buffer, int* fence_fd)
{
    auto self = static_cast<mga::MirNativeWindow*>(window);
    return self->dequeueBuffer(buffer, fence_fd);
}

int queueBuffer_deprecated_static(struct ANativeWindow* window,
                       struct ANativeWindowBuffer* buffer)
{
    auto self = static_cast<mga::MirNativeWindow*>(window);
    return self->queueBuffer(buffer, -1);
}

int queueBuffer_static(struct ANativeWindow* window,
                       struct ANativeWindowBuffer* buffer, int fence_fd)
{
    auto self = static_cast<mga::MirNativeWindow*>(window);
    return self->queueBuffer(buffer, fence_fd);
}

int setSwapInterval_static (struct ANativeWindow* window, int interval)
{
    auto self = static_cast<mga::MirNativeWindow*>(window);
    return self->setSwapInterval(interval);
}

/* lockBuffer, and cancelBuffer don't seem to being called by the driver. for now just return without calling into MirNativeWindow */

int lockBuffer_static(struct ANativeWindow* /*window*/,
                      struct ANativeWindowBuffer* /*buffer*/)
{
    return 0;
}

int cancelBuffer_deprecated_static(struct ANativeWindow* window,
                        struct ANativeWindowBuffer* buffer)
{
    return cancelBuffer_static(window, buffer, -1);
}

int cancelBuffer_static(struct ANativeWindow* window,
                        struct ANativeWindowBuffer* buffer, int fence_fd)
{
    auto self = static_cast<mga::MirNativeWindow*>(window);
    return self->cancelBuffer(buffer, fence_fd);
}
}

mga::MirNativeWindow::MirNativeWindow(std::shared_ptr<AndroidDriverInterpreter> const& interpreter)
 : driver_interpreter(interpreter), sync_ops(std::make_shared<mga::RealSyncFileOps>())
{
    ANativeWindow::query = &query_static;
    ANativeWindow::perform = &perform_static;
    ANativeWindow::setSwapInterval = &setSwapInterval_static;
    ANativeWindow::dequeueBuffer_DEPRECATED = &dequeueBuffer_deprecated_static;
    ANativeWindow::dequeueBuffer = &dequeueBuffer_static;
    ANativeWindow::lockBuffer_DEPRECATED = &lockBuffer_static;
    ANativeWindow::queueBuffer_DEPRECATED = &queueBuffer_deprecated_static;
    ANativeWindow::queueBuffer = &queueBuffer_static;
    ANativeWindow::cancelBuffer_DEPRECATED = &cancelBuffer_deprecated_static;
    ANativeWindow::cancelBuffer = &cancelBuffer_static;

    ANativeWindow::common.incRef = &incRef;
    ANativeWindow::common.decRef = &incRef;

    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;
}

int mga::MirNativeWindow::setSwapInterval(int interval)
try
{
    driver_interpreter->sync_to_display(interval != 0);

    return 0;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}

int mga::MirNativeWindow::dequeueBuffer(struct ANativeWindowBuffer** buffer_to_driver, int* fence_fd)
try
{
    if (cancelled_buffers.size() != 0)
    {
        *buffer_to_driver = cancelled_buffers.back();
        cancelled_buffers.pop_back();
        *fence_fd = -1; //no fence associated with cancelled buffers
    }
    else
    {
        auto buffer = driver_interpreter->driver_requests_buffer();

        //EGL driver is responsible for closing this native handle
        *fence_fd = buffer->copy_fence();
        *buffer_to_driver = buffer->anwb();
    }
    return 0;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}

int mga::MirNativeWindow::dequeueBufferAndWait(struct ANativeWindowBuffer** buffer_to_driver)
try
{
    if (cancelled_buffers.size() != 0)
    {
        *buffer_to_driver = cancelled_buffers.back();
        cancelled_buffers.pop_back();
    }
    else
    {
        auto buffer = driver_interpreter->driver_requests_buffer();
        *buffer_to_driver = buffer->anwb();
        buffer->ensure_available_for(mga::BufferAccess::write);
    }
    return 0;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}

int mga::MirNativeWindow::queueBuffer(struct ANativeWindowBuffer* buffer, int fence)
try
{
    driver_interpreter->driver_returns_buffer(buffer, fence);
    return 0;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}

int mga::MirNativeWindow::cancelBuffer(struct ANativeWindowBuffer* buffer, int fence)
try
{
    mga::SyncFence sync_fence(sync_ops, mir::Fd(fence));
    sync_fence.wait();

    cancelled_buffers.push_back(buffer);
    return 0;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}

int mga::MirNativeWindow::query(int key, int* value) const
try
{
    *value = driver_interpreter->driver_requests_info(key);
    return 0;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}

int mga::MirNativeWindow::perform(int key, va_list arg_list )
try
{
    int ret = 0;
    va_list args;
    va_copy(args, arg_list);

    switch(key)
    {
        case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
            driver_interpreter->dispatch_driver_request_format(va_arg(args, int));
            break;
        case NATIVE_WINDOW_SET_BUFFER_COUNT:
            driver_interpreter->dispatch_driver_request_buffer_count(va_arg(args, int));
            break;
        default:
            break;
    }

    va_end(args);
    return ret;
}
catch (std::exception const& e)
{
    MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(e);
    return -1;
}
