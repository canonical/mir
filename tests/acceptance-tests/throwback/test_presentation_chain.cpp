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

#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_toolkit/mir_buffer.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/geometry/size.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
using namespace testing;
using namespace std::chrono_literals;

namespace
{
struct Chain
{
    Chain(MirConnection* connection) :
        chain(mir_connection_create_presentation_chain_sync(connection))
    {
    }

    operator MirPresentationChain*()
    {
        return chain;
    }

    ~Chain()
    {
        mir_presentation_chain_release(chain);
    }
private:
    MirPresentationChain* chain;
};

class SurfaceWithChain
{
public:
    operator MirSurface*()
    {
        return surface;
    }

    Chain& chain()
    {
        return chain_;
    }

    ~SurfaceWithChain()
    {
        mir_surface_release_sync(surface);
    }
protected:
    SurfaceWithChain(MirConnection* connection, std::function<MirSurface*(Chain&)> const& fn) :
        chain_(connection),
        surface(fn(chain_))
    {
    }
private:
    Chain chain_;
    MirSurface* surface;
};

struct SurfaceWithChainFromStart : SurfaceWithChain
{
    SurfaceWithChainFromStart(MirConnection* connection, geom::Size size, MirPixelFormat pf) :
        SurfaceWithChain(connection,
        std::bind(&SurfaceWithChainFromStart::create_surface, this,
            std::placeholders::_1, connection, size, pf))
    {
    }
private:
    MirSurface* create_surface(Chain& chain, MirConnection* connection, geom::Size size, MirPixelFormat pf)
    {
        auto spec = mir_connection_create_spec_for_normal_surface(
            connection, size.width.as_int(), size.height.as_int(), pf);
        mir_surface_spec_add_presentation_chain(
            spec, size.width.as_int(), size.height.as_int(), 0, 0, chain);
        auto surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);
        return surface;
    }
};

struct SurfaceWithChainFromReassociation : SurfaceWithChain
{
    SurfaceWithChainFromReassociation(MirConnection* connection, geom::Size size, MirPixelFormat pf) :
        SurfaceWithChain(connection,
        std::bind(&SurfaceWithChainFromReassociation::create_surface, this,
            std::placeholders::_1, connection, size, pf))
    {
    }
private:
    MirSurface* create_surface(Chain& chain, MirConnection* connection, geom::Size size, MirPixelFormat pf)
    {
        MirSurfaceSpec* spec = mir_connection_create_spec_for_normal_surface(
            connection, size.width.as_int(), size.height.as_int(), pf);
        auto surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);
        spec = mir_create_surface_spec(connection);
        mir_surface_spec_add_presentation_chain(
            spec, size.width.as_int(), size.height.as_int(), 0, 0, chain);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);
        return surface;
    }
};

struct PresentationChain : mtf::ConnectedClientHeadlessServer
{
    geom::Size const size {100, 20};
    MirPixelFormat const pf = mir_pixel_format_abgr_8888;
    MirBufferUsage const usage = mir_buffer_usage_software;
    void SetUp() override
    {
        //test suite has to be run with the new semantics activated
        add_to_environment("MIR_SERVER_NBUFFERS", "0");
        ConnectedClientHeadlessServer::SetUp();
    }
};

struct MirBufferSync
{
    void buffer_available(MirBuffer* b)
    {
        std::unique_lock<std::mutex> lk(mutex);
        callback_count++;
        buffer_ = b;
        available = true;
        cv.notify_all();
    }

    bool wait_for_buffer(std::chrono::seconds timeout)
    {
        std::unique_lock<std::mutex> lk(mutex);
        return cv.wait_for(lk, timeout, [this] { return available; });
    }

    void unavailable()
    {
        std::unique_lock<std::mutex> lk(mutex);
        available = false;
    }

    MirBuffer* buffer()
    {
        return buffer_;
    }
private:
    std::mutex mutex;
    std::condition_variable cv;
    MirBuffer* buffer_ = nullptr;
    bool available = false;
    unsigned int callback_count = 0;
};

void buffer_callback(MirPresentationChain*, MirBuffer* buffer, void* context)
{
    auto sync = reinterpret_cast<MirBufferSync*>(context);
    sync->buffer_available(buffer);
}

}

TEST_F(PresentationChain, allocation_calls_callback)
{
    SurfaceWithChainFromStart surface(connection, size, pf);

    MirBufferSync context;
    mir_presentation_chain_allocate_buffer(
        surface.chain(),
        size.width.as_int(), size.height.as_int(), pf, usage,
        buffer_callback, &context);

    EXPECT_TRUE(context.wait_for_buffer(10s));
    EXPECT_THAT(context.buffer(), Ne(nullptr));    
}

TEST_F(PresentationChain, has_native_buffer)
{
    SurfaceWithChainFromStart surface(connection, size, pf);

    MirBufferSync context;
    mir_presentation_chain_allocate_buffer(
        surface.chain(),
        size.width.as_int(), size.height.as_int(), pf, usage,
        buffer_callback, &context);

    EXPECT_TRUE(context.wait_for_buffer(10s));
    auto buffer = context.buffer();
    EXPECT_THAT(context.buffer(), Ne(nullptr));

    //the native type for the stub platform is nullptr
    EXPECT_THAT(mir_buffer_get_native_buffer(buffer, mir_none), Eq(nullptr));
}

TEST_F(PresentationChain, has_native_fence)
{
    SurfaceWithChainFromStart surface(connection, size, pf);

    MirBufferSync context;
    mir_presentation_chain_allocate_buffer(
        surface.chain(),
        size.width.as_int(), size.height.as_int(), pf, usage,
        buffer_callback, &context);

    EXPECT_TRUE(context.wait_for_buffer(10s));
    auto buffer = context.buffer();
    EXPECT_THAT(context.buffer(), Ne(nullptr));

    //the native type for the stub platform is nullptr
    EXPECT_THAT(mir_buffer_get_fence(buffer), Eq(nullptr));
}

TEST_F(PresentationChain, can_map_for_cpu_render)
{
    SurfaceWithChainFromStart surface(connection, size, pf);

    MirBufferSync context;
    mir_presentation_chain_allocate_buffer(
        surface.chain(),
        size.width.as_int(), size.height.as_int(), pf, usage,
        buffer_callback, &context);

    EXPECT_TRUE(context.wait_for_buffer(10s));
    auto buffer = context.buffer();
    EXPECT_THAT(context.buffer(), Ne(nullptr));
    auto region = mir_buffer_get_graphics_region(buffer, mir_none);
    //cast to int so gtest doesn't try to print a char* that isn't a string
    EXPECT_THAT(reinterpret_cast<int*>(region.vaddr), Ne(nullptr));
    EXPECT_THAT(region.width, Eq(size.width.as_int()));
    EXPECT_THAT(region.height, Eq(size.height.as_int()));
    EXPECT_THAT(region.stride, Eq(size.width.as_int() * MIR_BYTES_PER_PIXEL(pf)));
    EXPECT_THAT(region.pixel_format, Eq(pf));
}

//needs an ABI break to fix
TEST_F(PresentationChain, DISABLED_submission_will_eventually_call_callback)
{
    SurfaceWithChainFromStart surface(connection, size, pf);

    auto const num_buffers = 2u;
    std::array<MirBufferSync, num_buffers> contexts;
    auto num_iterations = 50u;
    for(auto& context : contexts)
    {
        mir_presentation_chain_allocate_buffer(
            surface.chain(),
            size.width.as_int(), size.height.as_int(), pf, usage,
            buffer_callback, &context);
        ASSERT_TRUE(context.wait_for_buffer(10s));
        ASSERT_THAT(context.buffer(), Ne(nullptr));    
    }

    for(auto i = 0u; i < num_iterations; i++)
    {
        mir_presentation_chain_submit_buffer(surface.chain(), contexts[i % num_buffers].buffer());
        contexts[i % num_buffers].unavailable();
        if (i != 0)
            ASSERT_TRUE(contexts[(i-1) % num_buffers].wait_for_buffer(10s)) << "iteration " << i;
    }
}

TEST_F(PresentationChain, submission_will_eventually_call_callback_reassociated)
{
    SurfaceWithChainFromReassociation surface(connection, size, pf);

    auto const num_buffers = 2u;
    std::array<MirBufferSync, num_buffers> contexts;
    auto num_iterations = 50u;
    for(auto& context : contexts)
    {
        mir_presentation_chain_allocate_buffer(
            surface.chain(),
            size.width.as_int(), size.height.as_int(), pf, usage,
            buffer_callback, &context);
        ASSERT_TRUE(context.wait_for_buffer(10s));
        ASSERT_THAT(context.buffer(), Ne(nullptr));    
    }

    for(auto i = 0u; i < num_iterations; i++)
    {
        mir_presentation_chain_submit_buffer(surface.chain(), contexts[i % num_buffers].buffer());
        contexts[i % num_buffers].unavailable();
        if (i != 0)
            ASSERT_TRUE(contexts[(i-1) % num_buffers].wait_for_buffer(10s)) << "iteration " << i;
    }
}

TEST_F(PresentationChain, buffers_can_be_destroyed_before_theyre_returned)
{
    SurfaceWithChainFromStart surface(connection, size, pf);

    MirBufferSync context;
    mir_presentation_chain_allocate_buffer(
        surface.chain(),
        size.width.as_int(), size.height.as_int(), pf, usage,
        buffer_callback, &context);

    ASSERT_TRUE(context.wait_for_buffer(10s));
    ASSERT_THAT(context.buffer(), Ne(nullptr));
    mir_presentation_chain_submit_buffer(surface.chain(), context.buffer());
    mir_buffer_release(context.buffer());
}
