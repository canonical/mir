/* 
**
** Copyright 2007 The Android Open Source Project
**
** Licensed under the Apache License Version 2.0(the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing software 
** distributed under the License is distributed on an "AS IS" BASIS 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

//TODO: kdub remove this entire file and use mir native window types

#define LOG_TAG "FramebufferNativeWindow"
#define SYNC_IOC_WAIT 0x40043E00

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

//#include <cutils/log.h>
//#include <cutils/atomic.h>
//#include <utils/threads.h>
#include <utils/RefBase.h>

//#include <ui/ANativeObjectBase.h>
//#include <ui/Fence.h>
#include <ui/FramebufferNativeWindow.h>
#include <ui/Rect.h>

#include <EGL/egl.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

// ----------------------------------------------------------------------------
namespace android {
// ----------------------------------------------------------------------------

static void incRef(android_native_base_t*)
{
}
class NativeBuffer : public ANativeWindowBuffer 
{
public:


    NativeBuffer(int w, int h, int f, int u)  /*: BASE() */ 
    {
        ANativeWindowBuffer::width  = w;
        ANativeWindowBuffer::height = h;
        ANativeWindowBuffer::format = f;
        ANativeWindowBuffer::usage  = u;

        common.incRef = incRef;
        common.decRef = incRef; 
    }
    ~NativeBuffer() { }; // this class cannot be overloaded
};

/*
 * This implements the (main) framebuffer management. This class is used
 * mostly by SurfaceFlinger, but also by command line GL application.
 * 
 * In fact this is an implementation of ANativeWindow on top of
 * the framebuffer.
 * 
 * Currently it is pretty simple, it manages only two buffers (the front and 
 * back buffer).
 * 
 */

FramebufferNativeWindow::FramebufferNativeWindow() 
    :/* BASE(), */ fbDev(0), grDev(0), mUpdateOnDemand(false)
{
    hw_module_t const* module;

    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) == 0) {
        int err;
        int i;
        err = framebuffer_open(module, &fbDev);
        
        err = gralloc_open(module, &grDev);

        // bail out if we can't initialize the modules
        if (!fbDev || !grDev)
            return;
        
        mUpdateOnDemand = (fbDev->setUpdateRect != 0);
        
        mNumBuffers = 2; //fbDev->numFramebuffers;

        mNumFreeBuffers = mNumBuffers;
        mBufferHead = mNumBuffers-1;

        for (i = 0; i < mNumBuffers; i++)
        {
                buffers[i] = new NativeBuffer(
                        fbDev->width, fbDev->height, fbDev->format, GRALLOC_USAGE_HW_FB);
        }

        for (i = 0; i < mNumBuffers; i++)
        {
                err = grDev->alloc(grDev,
                        fbDev->width, fbDev->height, fbDev->format,
                        GRALLOC_USAGE_HW_FB, &buffers[i]->handle, &buffers[i]->stride);


                if (err)
                {
                        mNumBuffers = i;
                        mNumFreeBuffers = i;
                        mBufferHead = mNumBuffers-1;
                        break;
                }
        }

        const_cast<uint32_t&>(ANativeWindow::flags) = fbDev->flags; 
        const_cast<float&>(ANativeWindow::xdpi) = fbDev->xdpi;
        const_cast<float&>(ANativeWindow::ydpi) = fbDev->ydpi;
        const_cast<int&>(ANativeWindow::minSwapInterval) = 
            fbDev->minSwapInterval;
        const_cast<int&>(ANativeWindow::maxSwapInterval) = 
            fbDev->maxSwapInterval;
    }

    common.incRef = incRef;
    common.decRef = incRef; 
    ANativeWindow::setSwapInterval = setSwapInterval;
    ANativeWindow::dequeueBuffer = dequeueBuffer;
    ANativeWindow::queueBuffer = queueBuffer;
    ANativeWindow::query = query;
    ANativeWindow::perform = perform;
    ANativeWindow::cancelBuffer = NULL;

    ANativeWindow::dequeueBuffer_DEPRECATED = dequeueBuffer_DEPRECATED;
    ANativeWindow::lockBuffer_DEPRECATED = lockBuffer_DEPRECATED;
    ANativeWindow::queueBuffer_DEPRECATED = queueBuffer_DEPRECATED;
}

FramebufferNativeWindow::~FramebufferNativeWindow() 
{
    if (grDev) {
        for(int i = 0; i < mNumBuffers; i++) {
            if (buffers[i] != NULL) {
                grDev->free(grDev, buffers[i]->handle);
                delete buffers[i];
            }
        }
        gralloc_close(grDev);
    }

    if (fbDev) {
        framebuffer_close(fbDev);
    }
}

status_t FramebufferNativeWindow::setUpdateRectangle(const Rect& r) 
{
    if (!mUpdateOnDemand) {
        return INVALID_OPERATION;
    }
    return fbDev->setUpdateRect(fbDev, r.left, r.top, r.width(), r.height());
}

status_t FramebufferNativeWindow::compositionComplete()
{
    if (fbDev->compositionComplete) {
        return fbDev->compositionComplete(fbDev);
    }
    return INVALID_OPERATION;
}

int FramebufferNativeWindow::setSwapInterval(
        ANativeWindow* window, int interval) 
{
    framebuffer_device_t* fb =  static_cast<FramebufferNativeWindow*>(window)->fbDev;
    return fb->setSwapInterval(fb, interval);
}

void FramebufferNativeWindow::dump(String8& result) {
    if (fbDev->common.version >= 1 && fbDev->dump) {
        const size_t SIZE = 4096;
        char buffer[SIZE];

        fbDev->dump(fbDev, buffer, SIZE);
        result.append(buffer);
    }
}

// only for debugging / logging
int FramebufferNativeWindow::getCurrentBufferIndex() const
{
    return 0;
}

int FramebufferNativeWindow::dequeueBuffer_DEPRECATED(ANativeWindow* window, 
        ANativeWindowBuffer** buffer)
{
    int fenceFd = -1;
    int result = dequeueBuffer(window, buffer, &fenceFd);
    return result;
}

int FramebufferNativeWindow::dequeueBuffer(ANativeWindow* window, 
        ANativeWindowBuffer** buffer, int* fenceFd)
{
    FramebufferNativeWindow* self = static_cast<FramebufferNativeWindow*>(window);

    std::unique_lock<std::mutex> lk(self->mutex);
    while (self->mNumFreeBuffers < 2) {
        self->mCondition.wait(lk);
    }

    framebuffer_device_t* fb = self->fbDev;

    int index = self->mBufferHead++;
    if (self->mBufferHead >= self->mNumBuffers)
        self->mBufferHead = 0;

    // get this buffer
    self->mNumFreeBuffers--;
    self->mCurrentBufferIndex = index;

    *buffer = self->buffers[index];
    *fenceFd = -1;

    return 0;
}

int FramebufferNativeWindow::lockBuffer_DEPRECATED(ANativeWindow* window, 
        ANativeWindowBuffer* buffer)
{
    return NO_ERROR;
}

int FramebufferNativeWindow::queueBuffer_DEPRECATED(ANativeWindow* window, 
        ANativeWindowBuffer* buffer)
{
    return queueBuffer(window, buffer, -1);
}

int FramebufferNativeWindow::queueBuffer(ANativeWindow* window, 
        ANativeWindowBuffer* buffer, int fenceFd)
{
    FramebufferNativeWindow* self = static_cast<FramebufferNativeWindow*>(window);
    std::unique_lock<std::mutex> lk(self->mutex);

    framebuffer_device_t* fb = self->fbDev;
    buffer_handle_t handle = static_cast<NativeBuffer*>(buffer)->handle;

    if (fenceFd > 0)
    {
        close(fenceFd);
    }

    const int index = self->mCurrentBufferIndex;
    int res = fb->post(fb, handle);
    self->mNumFreeBuffers++;
    self->mCondition.notify_all();
    return res;
}

int FramebufferNativeWindow::query(const ANativeWindow* window,
        int what, int* value) 
{
    auto fbwin = const_cast<ANativeWindow*>(window);
    FramebufferNativeWindow* self = static_cast<FramebufferNativeWindow*>(fbwin);
    std::unique_lock<std::mutex> lk(self->mutex);

    framebuffer_device_t* fb = self->fbDev;
    switch (what) {
        case NATIVE_WINDOW_WIDTH:
            *value = fb->width;
            return NO_ERROR;
        case NATIVE_WINDOW_HEIGHT:
            *value = fb->height;
            return NO_ERROR;
        case NATIVE_WINDOW_FORMAT:
            *value = fb->format;
            return NO_ERROR;
        case NATIVE_WINDOW_CONCRETE_TYPE:
            *value = NATIVE_WINDOW_FRAMEBUFFER;
            return NO_ERROR;
        case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
            *value = 0;
            return NO_ERROR;
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            *value = fb->width;
            return NO_ERROR;
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            *value = fb->height;
            return NO_ERROR;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0;
            return NO_ERROR;
    }
    *value = 0;
    return BAD_VALUE;
}

int FramebufferNativeWindow::perform(ANativeWindow* window,
        int operation, ...)
{
    switch (operation) {
        case NATIVE_WINDOW_CONNECT:
        case NATIVE_WINDOW_DISCONNECT:
        case NATIVE_WINDOW_SET_USAGE:
        case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
        case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
        case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
        case NATIVE_WINDOW_API_CONNECT:
        case NATIVE_WINDOW_API_DISCONNECT:
            // TODO: we should implement these
            return NO_ERROR;

        case NATIVE_WINDOW_LOCK:
        case NATIVE_WINDOW_UNLOCK_AND_POST:
        case NATIVE_WINDOW_SET_CROP:
        case NATIVE_WINDOW_SET_BUFFER_COUNT:
        case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
        case NATIVE_WINDOW_SET_SCALING_MODE:
            return INVALID_OPERATION;
    }
    return NAME_NOT_FOUND;
}

// ----------------------------------------------------------------------------
}; // namespace android
// ----------------------------------------------------------------------------
