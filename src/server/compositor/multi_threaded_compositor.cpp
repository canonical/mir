/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/multi_threaded_compositor.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/compositor/compositing_strategy.h"

#include <atomic>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace mir
{
namespace compositor
{

class CompositingFunctor
{
public:
    CompositingFunctor(std::shared_ptr<mc::CompositingStrategy> const& strategy,
                       mg::DisplayBuffer& buffer)
        : compositing_strategy{strategy},
          buffer(buffer),
          running{true}
    {
    }

    void operator()()
    {
        while (running)
            compositing_strategy->render(buffer);
    }

    void stop()
    {
        running = false;
    }

private:
    std::shared_ptr<mc::CompositingStrategy> const compositing_strategy;
    mg::DisplayBuffer& buffer;
    std::atomic<bool> running;
};

}
}

mc::MultiThreadedCompositor::MultiThreadedCompositor(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::CompositingStrategy> const& strategy)
    : display{display},
      compositing_strategy{strategy}
{
}

mc::MultiThreadedCompositor::~MultiThreadedCompositor()
{
    stop();
}

void mc::MultiThreadedCompositor::start()
{
    display->for_each_display_buffer([=](mg::DisplayBuffer& buffer)
    {
        auto thread_functor_raw = new mc::CompositingFunctor{compositing_strategy, buffer};
        auto thread_functor = std::unique_ptr<mc::CompositingFunctor>(thread_functor_raw);

        threads.push_back(std::thread{std::ref(*thread_functor)});
        thread_functors.push_back(std::move(thread_functor));
    });
}

void mc::MultiThreadedCompositor::stop()
{
    for (auto& f : thread_functors)
        f->stop();

    for (auto& t : threads)
        t.join();

    thread_functors.clear();
    threads.clear();
}
