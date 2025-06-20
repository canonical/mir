/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMPOSITOR_MULTI_THREADED_COMPOSITOR_H_
#define MIR_COMPOSITOR_MULTI_THREADED_COMPOSITOR_H_

#include "mir/compositor/compositor.h"
#include "mir/geometry/forward.h"

#include <memory>
#include <vector>
#include <chrono>
#include <atomic>

namespace mir
{
namespace graphics
{
class Display;
class Cursor;
}
namespace scene
{
class Observer;
}

namespace compositor
{

class DisplayBufferCompositorFactory;
class DisplayListener;
class CompositingFunctor;
class Scene;
class CompositorReport;

enum class CompositorState
{
    started,
    stopped,
    starting,
    stopping
};

class MultiThreadedCompositor : public Compositor
{
public:
    MultiThreadedCompositor(
        std::shared_ptr<graphics::Display> const& display,
        std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory,
        std::shared_ptr<Scene> const& scene,
        std::shared_ptr<DisplayListener> const& display_listener,
        std::shared_ptr<CompositorReport> const& compositor_report,
        std::shared_ptr<graphics::Cursor> const& cursor,
        std::chrono::milliseconds fixed_composite_delay,  // -1 = automatic
        bool compose_on_start);
    ~MultiThreadedCompositor();

    void start();
    void stop();

private:
    void create_compositing_threads();
    void destroy_compositing_threads();

    std::shared_ptr<graphics::Display> const display;
    std::shared_ptr<DisplayBufferCompositorFactory> const display_buffer_compositor_factory;
    std::shared_ptr<Scene> const scene;
    std::shared_ptr<DisplayListener> const display_listener;
    std::shared_ptr<CompositorReport> const report;
    std::shared_ptr<graphics::Cursor> const cursor;

    std::vector<std::unique_ptr<CompositingFunctor>> thread_functors;

    std::atomic<CompositorState> state;
    std::chrono::milliseconds fixed_composite_delay;
    bool compose_on_start;

    void schedule_compositing();
    void schedule_compositing(geometry::Rectangle const& damage) const;

    std::shared_ptr<mir::scene::Observer> observer;
};

}
}

#endif /* MIR_COMPOSITOR_MULTI_THREADED_COMPOSITOR_H_ */
