/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/compositor.h"
#include "mir/graphics/renderable.h"

#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace
{
struct size_less
{
    bool operator() (geom::Size const& a, geom::Size const& b)
    {
        return a.width < b.width && a.height < b.height;
    }
};

struct CompositionTracker
{
    bool wait_until_surface_is_rendered_with_size(geom::Size sz)
    {
        using namespace std::literals::chrono_literals;
        std::unique_lock<decltype(mutex)> lk(mutex);
        return cv.wait_for(lk, 4s, [this, sz] {
            return std::find(rendered_sizes.begin(), rendered_sizes.end(), sz) != rendered_sizes.end();
        });
    }

    void surface_rendered_with_size(geom::Size sz)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        rendered_sizes.emplace(sz);
        cv.notify_all();
    }

private:
    std::set<geom::Size, size_less> rendered_sizes;
    std::mutex mutex;
    std::condition_variable cv;
};

struct CustomDBC : mc::DisplayBufferCompositor
{
    CustomDBC(std::shared_ptr<CompositionTracker> const& tracker) :
        tracker(tracker)
    {
    }

    void composite(mc::SceneElementSequence&& sequence) override
    {
        for(auto const& element : sequence)
        {
            auto renderable = element->renderable(); 
            renderable->buffer(); //consume buffer to stop compositor from spinning
            tracker->surface_rendered_with_size(renderable->screen_position().size);
        }
    }
private:
    std::shared_ptr<CompositionTracker> const tracker;
};

struct CustomDBCFactory : mc::DisplayBufferCompositorFactory
{
    CustomDBCFactory(std::shared_ptr<CompositionTracker> const& tracker) :
        tracker(tracker)
    {
    }
 
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer&) override
    {
        return std::make_unique<CustomDBC>(tracker);
    }
private:
    std::shared_ptr<CompositionTracker> const tracker;
};

struct DisplayBufferCompositorOverride : mtf::ConnectedClientWithASurface
{
    void SetUp() override
    {
        using namespace std::literals::chrono_literals;
        server.override_the_display_buffer_compositor_factory([this]
        {
            return std::make_shared<CustomDBCFactory>(tracker);
        });

        mtf::ConnectedClientWithASurface::SetUp();
    }
    void TearDown() override
    {
        mtf::ConnectedClientWithASurface::TearDown();
    }
protected:
    std::shared_ptr<CompositionTracker> const tracker{std::make_shared<CompositionTracker>()};
};
}

TEST_F(DisplayBufferCompositorOverride, composite_called_with_surface)
{
    MirSurfaceParameters surface_params;
    mir_surface_get_parameters(surface, &surface_params);
    mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    EXPECT_TRUE(tracker->wait_until_surface_is_rendered_with_size({surface_params.width, surface_params.height}));
}
