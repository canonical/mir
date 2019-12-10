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
      work{nullptr},
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

void mgc::EGLContextDelegate::run_in_egl_context(
    std::function<void()>&& functor)
{
    auto work_notifier = std::make_shared<std::promise<void>>();
    auto work_done = work_notifier->get_future();

    std::unique_lock<std::mutex> lock{mutex};
    if (work)
    {
        new_work.wait(lock, [this]() { return static_cast<bool>(work); });
    }

    work =
        [notifier = std::move(work_notifier), todo = std::move(functor)]() mutable
        {
            todo();
            notifier->set_value();
        };

    lock.unlock();
    new_work.notify_all();

    work_done.wait();
}

void mgc::EGLContextDelegate::process_loop(mgc::EGLContextDelegate* const me)
{
    me->ctx->make_current();

    std::unique_lock<std::mutex> lock{me->mutex};
    while (!me->shutdown_requested)
    {
        me->new_work.wait(lock);
        if (me->work)
        {
            me->work();
            me->work = nullptr;
        }
    }

    me->ctx->release_current();
}
