/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_gl_program_factory.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/renderer/gl/texture_source.h"
#include "src/server/report/null_report_factory.h"

#include "mir_test_framework/testing_server_configuration.h"

#include <EGL/egl.h>
#include <thread>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

namespace mir
{

mir::renderer::gl::TextureSource* as_texture_source(
    std::shared_ptr<mg::Buffer> const& buffer)
{
    return dynamic_cast<mir::renderer::gl::TextureSource*>(
        buffer->native_buffer_base());
}

class StubBufferThread : public mtd::StubBuffer,
                         public mir::renderer::gl::TextureSource
{
public:
    StubBufferThread() :
        creation_thread_id{std::this_thread::get_id()}
    {}

    void gl_bind_to_texture() override
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

    void bind() override { gl_bind_to_texture(); }
    void secure_for_render() override {}

private:
    std::thread::id creation_thread_id;
};

class StubGraphicBufferAllocator : public mtd::StubBufferAllocator
{
 public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&) override
    {
        return std::make_shared<StubBufferThread>();
    }
};

class StubGraphicPlatform : public mtd::NullPlatform
{
public:
    UniqueModulePtr<mg::GraphicBufferAllocator>
        create_buffer_allocator(mg::Display const&) override
    {
        return mir::make_module_ptr<StubGraphicBufferAllocator>();
    }
};

class MesaBufferIntegration : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        platform = std::make_shared<StubGraphicPlatform>();

        auto conf_policy = std::make_shared<mg::CloneDisplayConfigurationPolicy>();
        display = platform->create_display(
            conf_policy,
            std::make_shared<mtd::StubGLConfig>());
        allocator = platform->create_buffer_allocator(*display);
        size = geom::Size{100, 100};
        pf = mir_pixel_format_abgr_8888;
        usage = mg::BufferUsage::hardware;
        buffer_properties = mg::BufferProperties{size, pf, usage};
    }

    std::shared_ptr<mg::Platform> platform;
    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    geom::Size size;
    MirPixelFormat pf;
    mg::BufferUsage usage;
    mg::BufferProperties buffer_properties;
};

struct BufferCreatorThread
{
    BufferCreatorThread(const std::shared_ptr<mg::GraphicBufferAllocator>& allocator,
                        mg::BufferProperties const& buffer_properties)
        : allocator{allocator}, buffer_properties{buffer_properties}
    {
    }

    void operator()()
    {
        using namespace testing;
        buffer = allocator->alloc_buffer(buffer_properties);
    }

    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    std::shared_ptr<mg::Buffer> buffer;
    mg::BufferProperties buffer_properties;
};

struct BufferDestructorThread
{
    BufferDestructorThread(std::shared_ptr<mg::Buffer> buffer)
        : buffer{std::move(buffer)}
    {
    }

    void operator()()
    {
        using namespace testing;
        buffer.reset();
        ASSERT_EQ(EGL_SUCCESS, eglGetError());
    }

    std::shared_ptr<mg::Buffer> buffer;
};

struct BufferTextureInstantiatorThread
{
    BufferTextureInstantiatorThread(const std::shared_ptr<mg::Buffer>& buffer)
        : buffer(buffer), exception_thrown(false)
    {
    }

    void operator()()
    {
        using namespace testing;

        try
        {
            as_texture_source(buffer)->gl_bind_to_texture();
        }
        catch(std::runtime_error const&)
        {
            exception_thrown = true;
        }

        ASSERT_NE(EGL_SUCCESS, eglGetError());
    }

    const std::shared_ptr<mg::Buffer>& buffer;
    bool exception_thrown;
};

TEST_F(MesaBufferIntegration, buffer_creation_from_arbitrary_thread_works)
{
    using namespace testing;

    EXPECT_NO_THROW({
        BufferCreatorThread creator(allocator, buffer_properties);
        std::thread t{std::ref(creator)};
        t.join();
        ASSERT_TRUE(creator.buffer.get() != 0);
    });
}

TEST_F(MesaBufferIntegration, buffer_destruction_from_arbitrary_thread_works)
{
    using namespace testing;

    EXPECT_NO_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
        as_texture_source(buffer)->gl_bind_to_texture();
        ASSERT_EQ(EGL_SUCCESS, eglGetError());

        BufferDestructorThread destructor{std::move(buffer)};
        std::thread t{std::ref(destructor)};
        t.join();
    });
}

TEST_F(MesaBufferIntegration, buffer_lazy_texture_instantiation_from_arbitrary_thread_fails)
{
    using namespace testing;

    EXPECT_NO_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
        BufferTextureInstantiatorThread texture_instantiator{buffer};
        std::thread t{std::ref(texture_instantiator)};
        t.join();
        ASSERT_TRUE(texture_instantiator.exception_thrown);
    });
}
}
