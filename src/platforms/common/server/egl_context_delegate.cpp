/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "egl_context_delegate.h"
#include "mir/renderer/gl/context.h"

namespace mgc = mir::graphics::common;

mgc::EGLContextDelegate::EGLContextDelegate(
    std::unique_ptr<mir::renderer::gl::Context> context)
    : ctx{std::move(context)},
      egl_thread(std::thread{process_loop, this})
{
}

mgc::EGLContextDelegate::~EGLContextDelegate() noexcept
{
    {
        std::lock_guard<std::mutex> lock{mutex};
        shutdown_requested = true;
    }
    new_work.notify_all();
    egl_thread.join();
}

void mgc::EGLContextDelegate::defer_to_egl_context(
    std::function<void()>&& functor)
{
    {
        std::lock_guard<std::mutex> lock{mutex};
        work_queue.emplace_back(functor);
    }
    new_work.notify_all();
}

void mgc::EGLContextDelegate::process_loop(mgc::EGLContextDelegate* const me)
{
    me->ctx->make_current();

    std::unique_lock<std::mutex> lock{me->mutex};
    while (!me->shutdown_requested)
    {
        me->new_work.wait(lock);
        for (auto& work : me->work_queue)
        {
            work();
        }
        me->work_queue.clear();
    }

    me->ctx->release_current();
}
