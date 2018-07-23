/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"

#include "mir/compositor/compositor.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_id.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/doubles/stub_renderer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

struct StubRenderer : mtd::StubRenderer
{
    void render(mg::RenderableList const& renderables) const override
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto const& r : renderables)
            rendered_buffers_.push_back(r->buffer()->id());

        if (renderables.size() > 0)
            new_rendered_buffer_cv.notify_all();
    }

    std::vector<mg::BufferID> rendered_buffers()
    {
        std::lock_guard<std::mutex> lock{mutex};
        return rendered_buffers_;
    }

    std::vector<mg::BufferID> wait_for_new_rendered_buffers()
    {
        std::unique_lock<std::mutex> lock{mutex};

        new_rendered_buffer_cv.wait_for(
            lock, std::chrono::seconds{2},
            [this] { return rendered_buffers_.size() != 0; });

        auto const rendered = std::move(rendered_buffers_);
        return rendered;
    }

    mutable std::mutex mutex;
    mutable std::condition_variable new_rendered_buffer_cv;
    mutable std::vector<mg::BufferID> rendered_buffers_;
};

class StubRendererFactory : public mir::renderer::RendererFactory
{
public:
    std::unique_ptr<mir::renderer::Renderer> create_renderer_for(
        mg::DisplayBuffer&) override
    {
        std::lock_guard<std::mutex> lock{mutex};
        renderer_ = new StubRenderer();
        renderer_created_cv.notify_all();
        return std::unique_ptr<mir::renderer::Renderer>{renderer_};
    }

    StubRenderer* renderer()
    {
        std::unique_lock<std::mutex> lock{mutex};

        renderer_created_cv.wait_for(
            lock, std::chrono::seconds{2},
            [this] { return renderer_ != nullptr; });

        return renderer_;
    }

    void clear_renderer()
    {
        std::lock_guard<std::mutex> lock{mutex};
        renderer_ = nullptr;
    }

    std::mutex mutex;
    std::condition_variable renderer_created_cv;
    StubRenderer* renderer_ = nullptr;
};

class PostObserver : public mir::scene::NullSurfaceObserver
{
public:
    PostObserver(std::function<void(int)> cb) : cb{cb}
    {
    }

    void frame_posted(mir::scene::Surface const*, int count, geom::Size const&) override
    {
        cb(count);
    }

private:
    std::function<void(int)> const cb;
};

class PublicSurfaceFactory : public mir::scene::SurfaceFactory
{
public:
    using Surface = mir::scene::Surface;

    PublicSurfaceFactory(std::shared_ptr<mir::scene::SurfaceFactory> const& real) : real_surface_factory{real} {}

    std::shared_ptr<Surface> create_surface(
        std::list<mir::scene::StreamInfo> const& streams,
        mir::scene::SurfaceCreationParameters const& params) override
    {
        latest_surface = real_surface_factory->create_surface(streams, params);
        return latest_surface;
    }

    std::shared_ptr<Surface> const latest() const
    {
        return latest_surface;
    }

    void forget_latest()
    {
        latest_surface.reset();
    }

private:
    std::shared_ptr<mir::scene::SurfaceFactory> const real_surface_factory;
    std::shared_ptr<Surface> latest_surface;
};

struct StubServerConfig : mtf::StubbedServerConfiguration
{
    std::shared_ptr<PublicSurfaceFactory> the_public_surface_factory()
    {
        return public_surface_factory(
                   [this]{ return std::make_shared<PublicSurfaceFactory>(
                           mtf::StubbedServerConfiguration::the_surface_factory()); });
    }

    std::shared_ptr<mir::scene::SurfaceFactory> the_surface_factory() override
    {
        return the_public_surface_factory();
    }

    std::shared_ptr<StubRendererFactory> the_stub_renderer_factory()
    {
        return stub_renderer_factory(
            [] { return std::make_shared<StubRendererFactory>(); });
    }

    std::shared_ptr<mir::renderer::RendererFactory> the_renderer_factory() override
    {
        return the_stub_renderer_factory();
    }

    mir::CachedPtr<StubRendererFactory> stub_renderer_factory;
    mir::CachedPtr<PublicSurfaceFactory> public_surface_factory;
};

using BasicFixture = mtf::BasicClientServerFixture<StubServerConfig>;

struct StaleFrames : BasicFixture,
                     ::testing::WithParamInterface<int>
{
    StaleFrames()
        : post_observer(std::make_shared<PostObserver>(
            [this](int n){frame_posted(n);}))
    {
    }

    void SetUp()
    {
        BasicFixture::SetUp();

        client_create_surface();
        auto pub = server_configuration.the_public_surface_factory();
        auto surface = pub->latest();
        ASSERT_TRUE(!!surface);
        surface->add_observer(post_observer);
        pub->forget_latest();
    }

    void TearDown()
    {
        mir_window_release_sync(window);

        BasicFixture::TearDown();
    }

    void client_create_surface()
    {
        window = mtf::make_any_surface(connection);
        ASSERT_TRUE(mir_window_is_valid(window));
    }

    std::vector<mg::BufferID> wait_for_new_rendered_buffers()
    {
        return server_configuration.the_stub_renderer_factory()->renderer()->wait_for_new_rendered_buffers();
    }

    void stop_compositor()
    {
        server_configuration.the_compositor()->stop();
        server_configuration.the_stub_renderer_factory()->clear_renderer();
    }

    void start_compositor()
    {
        server_configuration.the_compositor()->start();
    }

    /*
     * NOTE that we wait for surface buffer posts as opposed to display posts.
     * The difference is that surface buffer posts will precisely match the
     * number of client swaps for any swap interval, but display posts may be
     * fewer than the number of swaps if the client was quick and using
     * interval zero.
     */
    void frame_posted(int count)
    {
        std::unique_lock<std::mutex> lock(mutex);
        posts += count;
        posted.notify_all();
    }

    bool wait_for_posts(int count, std::chrono::seconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto const deadline = std::chrono::steady_clock::now() + timeout;
        while (posts < count)
        {
            if (posted.wait_until(lock, deadline) == std::cv_status::timeout)
                return false;
        }
        posts -= count;
        return true;
    }

    MirWindow* window;

private:
    std::shared_ptr<PostObserver> post_observer;
    std::mutex mutex;
    std::condition_variable posted;
    int posts = 0;
};

}

TEST_P(StaleFrames, are_dropped_when_restarting_compositor)
{
    using namespace testing;

    stop_compositor();

    std::set<mg::BufferID> stale_buffers;

    stale_buffers.emplace(mir_debug_window_current_buffer_id(window));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto bs = mir_window_get_buffer_stream(window);
#pragma GCC diagnostic pop
    mir_buffer_stream_set_swapinterval(bs, GetParam());
    mir_buffer_stream_swap_buffers_sync(bs);

    stale_buffers.emplace(mir_debug_window_current_buffer_id(window));
    mir_buffer_stream_swap_buffers_sync(bs);

    EXPECT_THAT(stale_buffers.size(), Eq(2u));

    auto const fresh_buffer = mg::BufferID{mir_debug_window_current_buffer_id(window)};
    mir_buffer_stream_swap_buffers_sync(bs);

    ASSERT_TRUE(wait_for_posts(3, 60s));
    start_compositor();

    // Note first stale buffer and fresh_buffer may be equal when defaulting to double buffers
    stale_buffers.erase(fresh_buffer);

    auto const new_buffers = wait_for_new_rendered_buffers();
    ASSERT_THAT(new_buffers.size(), Eq(1u));
    EXPECT_THAT(stale_buffers, Not(Contains(new_buffers[0])));
}

TEST_P(StaleFrames, only_fresh_frames_are_used_after_restarting_compositor)
{
    using namespace testing;

    stop_compositor();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto bs = mir_window_get_buffer_stream(window);
#pragma GCC diagnostic pop
    mir_buffer_stream_set_swapinterval(bs, GetParam());
    mir_buffer_stream_swap_buffers_sync(bs);
    mir_buffer_stream_swap_buffers_sync(bs);

    auto const fresh_buffer = mg::BufferID{mir_debug_window_current_buffer_id(window)};
    mir_buffer_stream_swap_buffers_sync(bs);

    ASSERT_TRUE(wait_for_posts(3, 60s));
    start_compositor();

    auto const new_buffers = wait_for_new_rendered_buffers();
    ASSERT_THAT(new_buffers.size(), Eq(1u));
    EXPECT_THAT(new_buffers[0], Eq(fresh_buffer));
}

INSTANTIATE_TEST_CASE_P(PerSwapInterval, StaleFrames, ::testing::Values(0,1));
