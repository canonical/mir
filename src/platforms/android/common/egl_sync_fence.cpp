/*
 * Copyright © 2015 Canonical Ltd.
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

#include "egl_sync_fence.h"
#include <boost/throw_exception.hpp>
#include <sstream>

namespace mg = mir::graphics;

void mg::NullCommandSync::raise()
{
}

void mg::NullCommandSync::reset()
{
}

bool mg::NullCommandSync::wait_for(std::chrono::nanoseconds)
{
    return true;
}

mg::EGLSyncFence::EGLSyncFence(std::shared_ptr<EGLSyncExtensions> const& egl) :
    egl(egl),
    fence_display(EGL_NO_DISPLAY),
    sync_point(EGL_NO_SYNC_KHR)
{
}

mg::EGLSyncFence::~EGLSyncFence()
{
    reset();
}

void mg::EGLSyncFence::raise()
{
    std::unique_lock<std::mutex> lk(mutex);
    wait_for(lk, default_timeout);

    fence_display = eglGetCurrentDisplay();
    sync_point = egl->eglCreateSyncKHR(fence_display, EGL_SYNC_FENCE_KHR, NULL);
    if (sync_point == EGL_NO_SYNC_KHR)
    {
        std::stringstream str;
        str << "failed to add sync point to command buffer" << eglGetError();
        BOOST_THROW_EXCEPTION(std::runtime_error(str.str()));
    }
}

void mg::EGLSyncFence::reset()
{
    std::unique_lock<std::mutex> lk(mutex);
    reset(lk);
}

void mg::EGLSyncFence::reset(std::unique_lock<std::mutex> const&)
{
    if (sync_point != EGL_NO_SYNC_KHR)
    {
        egl->eglDestroySyncKHR(fence_display, sync_point);
        fence_display = EGL_NO_DISPLAY;
        sync_point = EGL_NO_SYNC_KHR;
    }
}

bool mg::EGLSyncFence::wait_for(std::chrono::nanoseconds ns)
{
    std::unique_lock<std::mutex> lk(mutex);
    return wait_for(lk, ns);
}

bool mg::EGLSyncFence::wait_for(
    std::unique_lock<std::mutex> const& lk,
    std::chrono::nanoseconds ns)
{
    auto status = EGL_CONDITION_SATISFIED_KHR;
    if (sync_point != EGL_NO_SYNC_KHR)
    {
        status = egl->eglClientWaitSyncKHR(fence_display, sync_point, 0, ns.count());
        reset(lk);
    }
    return status == EGL_CONDITION_SATISFIED_KHR;
}
