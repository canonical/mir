/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/gbm/gbm_platform.h"
#include "mir/graphics/gbm/gbm_display.h"
#include "mir/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/compositor/buffer.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/thread/all.h"

#include "mir_test/test_utils_config.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mt = mir::test;

namespace
{

class StubBuffer : public mc::Buffer
{
public:
    StubBuffer() : creation_thread_id{std::this_thread::get_id()} {}

    geom::Size size() const { return geom::Size(); }

    geom::Stride stride() const { return geom::Stride(); }

    geom::PixelFormat pixel_format() const { return geom::PixelFormat(); }

    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const { return std::shared_ptr<mc::BufferIPCPackage>(); }

    void bind_to_texture()
    {
        /*
         * If we are trying to bind the texture from a different thread from
         * the one used to create the buffer (i.e. a thread in which the
         * display is not supposed to be configured), force an EGL error to
         * make the tests happy.
         */
        if (std::this_thread::get_id() != creation_thread_id)
        {
            eglInitialize(0, 0, 0);
            throw std::runtime_error("Binding to texture failed");
        }
    }

private:
    std::thread::id creation_thread_id;
};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    std::unique_ptr<mc::Buffer> alloc_buffer(geom::Size, geom::PixelFormat)
    {
        return std::unique_ptr<mc::Buffer>(new StubBuffer());
    }
};

class StubGraphicPlatform : public mg::Platform
{
public:
    virtual std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    virtual std::shared_ptr<mg::Display> create_display()
    {
        return std::shared_ptr<mg::Display>();
    }

    virtual std::shared_ptr<mg::PlatformIPCPackage> get_ipc_package()
    {
        return std::shared_ptr<mg::PlatformIPCPackage>();
    }
};

}

class GBMBufferIntegration : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mt::Config config;

        if (config.use_real_graphics())
            platform = mg::create_platform();
        else
            platform = std::make_shared<StubGraphicPlatform>();

        display = platform->create_display();
        auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
        allocator = platform->create_buffer_allocator(buffer_initializer);
        size = geom::Size{geom::Width{100}, geom::Height{100}};
        pf = geom::PixelFormat::rgba_8888;
    }

    std::shared_ptr<mg::Platform> platform;
    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator;
    geom::Size size;
    geom::PixelFormat pf;

};

struct BufferCreatorThread
{
    BufferCreatorThread(const std::shared_ptr<mc::GraphicBufferAllocator>& allocator,
                        const geom::Size& size, const geom::PixelFormat& pf)
        : allocator{allocator}, size{size}, pf{pf}
    {
    }

    void operator()()
    {
        using namespace testing;
        buffer = allocator->alloc_buffer(size, pf);
    }

    std::shared_ptr<mc::GraphicBufferAllocator> allocator;
    std::unique_ptr<mc::Buffer> buffer;
    geom::Size size;
    geom::PixelFormat pf;
};

struct BufferDestructorThread
{
    BufferDestructorThread(std::unique_ptr<mc::Buffer> buffer)
        : buffer{std::move(buffer)}
    {
    }

    void operator()()
    {
        using namespace testing;
        buffer.reset(0);
        ASSERT_EQ(EGL_SUCCESS, eglGetError());
    }

    std::unique_ptr<mc::Buffer> buffer;
};

struct BufferTextureInstantiatorThread
{
    BufferTextureInstantiatorThread(const std::unique_ptr<mc::Buffer>& buffer)
        : buffer(buffer), exception_thrown(false)
    {
    }

    void operator()()
    {
        using namespace testing;

        try
        {
            buffer->bind_to_texture();
        }
        catch(std::runtime_error)
        {
            exception_thrown = true;
        }

        ASSERT_NE(EGL_SUCCESS, eglGetError());
    }

    const std::unique_ptr<mc::Buffer>& buffer;
    bool exception_thrown;
};

TEST_F(GBMBufferIntegration, buffer_creation_from_arbitrary_thread_works)
{
    using namespace testing;

    EXPECT_NO_THROW({
        BufferCreatorThread creator(allocator, size, pf);
        std::thread t{std::ref(creator)};
        t.join();
        ASSERT_TRUE(creator.buffer.get() != 0);
    });
}

TEST_F(GBMBufferIntegration, buffer_destruction_from_arbitrary_thread_works)
{
    using namespace testing;

    EXPECT_NO_THROW({
        auto buffer = allocator->alloc_buffer(size, pf);
        buffer->bind_to_texture();
        ASSERT_EQ(EGL_SUCCESS, eglGetError());

        BufferDestructorThread destructor{std::move(buffer)};
        std::thread t{std::ref(destructor)};
        t.join();
    });
}

TEST_F(GBMBufferIntegration, buffer_lazy_texture_instantiation_from_arbitrary_thread_fails)
{
    using namespace testing;

    EXPECT_NO_THROW({
        auto buffer = allocator->alloc_buffer(size, pf);
        BufferTextureInstantiatorThread texture_instantiator{buffer};
        std::thread t{std::ref(texture_instantiator)};
        t.join();
        ASSERT_TRUE(texture_instantiator.exception_thrown);
    });
}
