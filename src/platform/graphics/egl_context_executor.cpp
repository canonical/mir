/*
 * Copyright © Canonical Ltd.
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
 */

#include <mir/graphics/egl_context_executor.h>
#include <mir/renderer/gl/context.h>

namespace mgc = mir::graphics::common;

mgc::EGLContextExecutor::EGLContextExecutor(
    std::unique_ptr<mir::renderer::gl::Context> context)
    : ctx{std::move(context)},
      egl_thread(std::thread{process_loop, this})
{
}

mgc::EGLContextExecutor::~EGLContextExecutor() noexcept
{
    {
        std::lock_guard lock{mutex};
        shutdown_requested = true;
    }
    new_work.notify_one();
    egl_thread.join();
}

void mgc::EGLContextExecutor::spawn(
    std::function<void()>&& functor)
{
    {
        std::lock_guard lock{mutex};
        work_queue.emplace_back(std::move(functor));
    }
    new_work.notify_one();
}

void mgc::EGLContextExecutor::process_loop(mgc::EGLContextExecutor* const me)
{
    me->ctx->make_current();

    std::unique_lock lock{me->mutex};
    while (!me->shutdown_requested)
    {
        for (auto& work : me->work_queue)
        {
            work();
        }
        me->work_queue.clear();

        me->new_work.wait(lock);
    }

    // Drain the work-queue
    for (auto& work : me->work_queue)
    {
        work();
    }
    // …and ensure any functor cleanup happens with the EGL context current, too.
    me->work_queue.clear();

    me->ctx->release_current();
}
