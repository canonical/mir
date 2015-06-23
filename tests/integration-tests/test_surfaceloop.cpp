/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_sync_group.h"
#include "mir/test/doubles/stub_display_buffer.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir/test/gmock_fixes.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
geom::Size const size{640, 480};
MirPixelFormat const format{mir_pixel_format_abgr_8888};
mg::BufferUsage const usage{mg::BufferUsage::hardware};
mg::BufferProperties const buffer_properties{size, format, usage};


class MockGraphicBufferAllocator : public mtd::StubBufferAllocator
{
 public:
    MockGraphicBufferAllocator()
    {
        using testing::_;
        ON_CALL(*this, alloc_buffer(_))
            .WillByDefault(testing::Invoke(this, &MockGraphicBufferAllocator::on_create_swapper));
    }

    MOCK_METHOD1(
        alloc_buffer,
        std::shared_ptr<mg::Buffer> (mg::BufferProperties const&));


    std::shared_ptr<mg::Buffer> on_create_swapper(mg::BufferProperties const&)
    {
        return std::make_shared<mtd::StubBuffer>(::buffer_properties);
    }

    ~MockGraphicBufferAllocator() noexcept {}
};

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay() :
        display_sync_group{geom::Size{1600,1600}}
    {
    }

    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f) override
    {
        f(display_sync_group);
    }

private:
    mtd::StubDisplaySyncGroup display_sync_group;
};

struct SurfaceSync
{
    void surface_created(MirSurface * new_surface)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = new_surface;
        wait_condition.notify_all();
    }

    void surface_released(MirSurface * /*released_surface*/)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = NULL;
        wait_condition.notify_all();
    }

    void wait_for_surface_create()
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.wait(lock, [&]{ return !!surface; });
    }

    void wait_for_surface_release()
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.wait(lock, [&]{ return !surface; });
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    MirSurface * surface{nullptr};
};

void create_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_created(surface);
}

void release_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_released(surface);
}

void wait_for_surface_create(SurfaceSync* context)
{
    context->wait_for_surface_create();
}

void wait_for_surface_release(SurfaceSync* context)
{
    context->wait_for_surface_release();
}

struct BufferCounterConfig : mtf::StubbedServerConfiguration
{
    class CountingStubBuffer : public mtd::StubBuffer
    {
    public:

        CountingStubBuffer()
        {
            std::lock_guard<std::mutex> lock{buffers_mutex};
            ++buffers_created;
            buffers_cv.notify_one();
        }
        ~CountingStubBuffer()
        {
            std::lock_guard<std::mutex> lock{buffers_mutex};
            ++buffers_destroyed;
            buffers_cv.notify_one();
        }

        static std::mutex buffers_mutex;
        static std::condition_variable buffers_cv;
        static int buffers_created;
        static int buffers_destroyed;
    };

    class StubGraphicBufferAllocator : public mtd::StubBufferAllocator
    {
     public:
        std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&) override
        {
            return std::make_shared<CountingStubBuffer>();
        }
    };

    class StubPlatform : public mtd::NullPlatform
    {
    public:
        std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator() override
        {
            return std::make_shared<StubGraphicBufferAllocator>();
        }

        std::shared_ptr<mg::Display> create_display(
            std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
            std::shared_ptr<mg::GLProgramFactory> const&,
            std::shared_ptr<mg::GLConfig> const&) override
        {
            return std::make_shared<StubDisplay>();
        }
    };

    std::shared_ptr<mg::Platform> the_graphics_platform()
    {
        if (!platform)
            platform = std::make_shared<StubPlatform>();

        return platform;
    }

    std::shared_ptr<mg::Platform> platform;
};

std::mutex BufferCounterConfig::CountingStubBuffer::buffers_mutex;
std::condition_variable BufferCounterConfig::CountingStubBuffer::buffers_cv;
int BufferCounterConfig::CountingStubBuffer::buffers_created;
int BufferCounterConfig::CountingStubBuffer::buffers_destroyed;
}

struct SurfaceLoop : mtf::BasicClientServerFixture<BufferCounterConfig>
{
    static const int max_surface_count = 5;
    SurfaceSync ssync[max_surface_count];
    MirSurfaceSpec* surface_spec;

    void SetUp() override
    {
        mtf::BasicClientServerFixture<BufferCounterConfig>::SetUp();
        surface_spec = mir_connection_create_spec_for_normal_surface(
            connection, 640, 480, mir_pixel_format_abgr_8888);
    }

    void TearDown() override
    {
        mir_surface_spec_release(surface_spec);

        mtf::BasicClientServerFixture<BufferCounterConfig>::TearDown();

        using  Counter = BufferCounterConfig::CountingStubBuffer;

        std::chrono::seconds const long_enough{4};
        auto const all_buffers_destroyed =
            []{ return Counter::buffers_created == Counter::buffers_destroyed; };
        std::unique_lock<std::mutex> lock{Counter::buffers_mutex};

        EXPECT_TRUE(
            Counter::buffers_cv.wait_for(lock, long_enough, all_buffers_destroyed));
    }
};

TEST_F(SurfaceLoop, all_created_buffers_are_destroyed)
{
    for (int i = 0; i != max_surface_count; ++i)
        mir_surface_create(surface_spec, create_surface_callback, ssync+i);

    for (int i = 0; i != max_surface_count; ++i)
        wait_for_surface_create(ssync+i);

    for (int i = 0; i != max_surface_count; ++i)
        mir_surface_release(ssync[i].surface, release_surface_callback, ssync+i);

    for (int i = 0; i != max_surface_count; ++i)
        wait_for_surface_release(ssync+i);
}

TEST_F(SurfaceLoop, all_created_buffers_are_destroyed_if_client_disconnects_without_releasing_surfaces)
{
    for (int i = 0; i != max_surface_count; ++i)
        mir_surface_create(surface_spec, create_surface_callback, ssync+i);

    for (int i = 0; i != max_surface_count; ++i)
        wait_for_surface_create(ssync+i);
}
