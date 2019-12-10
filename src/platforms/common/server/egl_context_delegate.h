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

#ifndef MIR_EGL_CONTEXT_DELEGATE_H
#define MIR_EGL_CONTEXT_DELEGATE_H

#include <memory>
#include <future>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace mir
{
namespace renderer
{
namespace gl
{
class Context;
}
}

namespace graphics
{
namespace common
{
class EGLContextDelegate
{
public:
    EGLContextDelegate(std::unique_ptr<renderer::gl::Context> context);
    ~EGLContextDelegate() noexcept;

    void run_in_egl_context(std::function<void()>&& functor);
private:
    static void process_loop(EGLContextDelegate* const me);

    std::unique_ptr<renderer::gl::Context> const ctx;
    std::mutex mutex;
    std::condition_variable new_work;
    std::function<void()> work;
    bool shutdown_requested{false};

    std::thread egl_thread;
};

}
}
}

#endif //MIR_EGL_CONTEXT_DELEGATE_H
