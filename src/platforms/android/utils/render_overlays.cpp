/*
 * Copyright © 2012, 2014, 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/options/default_configuration.h"
#include "mir/shared_library_prober.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_probe.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/display_report.h"
#include "mir/renderer/gl/render_target.h"
#include "mir_image.h"
#include "as_render_target.h"
#include "mir/logging/null_shared_library_prober_report.h"
#include "GLES2/gl2.h"
#include <chrono>
#include <csignal>
#include <iostream>

namespace mg=mir::graphics;
namespace mo=mir::options;
namespace geom=mir::geometry;
namespace me=mir::examples;

namespace
{
volatile std::sig_atomic_t running = true;

void signal_handler(int /*signum*/)
{
    running = false;
}
class PixelBufferABGR
{
public:
    PixelBufferABGR(geom::Size sz, uint32_t color) :
        size{sz.width.as_uint32_t() * sz.height.as_uint32_t()},
        data{new uint32_t[size]}
    {
        fill(color);
    }

    void fill(uint32_t color)
    {
        for(auto i = 0u; i < size; i++)
            data[i] = color;
    }

    unsigned char* pixels()
    {
        return reinterpret_cast<unsigned char*>(data.get());
    }

    size_t pixel_size()
    {
        return size * sizeof(uint32_t);
    }
private:
    size_t size;
    std::unique_ptr<uint32_t[]> data;
};

class DemoOverlayClient
{
public:
    DemoOverlayClient(
        mg::GraphicBufferAllocator& buffer_allocator,
        mg::BufferProperties const& buffer_properties, uint32_t color)
         : front_buffer(buffer_allocator.alloc_buffer(buffer_properties)),
           back_buffer(buffer_allocator.alloc_buffer(buffer_properties)),
           color{color},
           last_tick{std::chrono::high_resolution_clock::now()},
           pixel_buffer{buffer_properties.size, color}
    {
    }

    void update_green_channel()
    {
        char green_value = (color >> 8) & 0xFF;
        green_value += compute_update_value();
        color &= 0xFFFF00FF;
        color |= (green_value << 8);
        pixel_buffer.fill(color);

        back_buffer->write(pixel_buffer.pixels(), pixel_buffer.pixel_size());
        std::swap(front_buffer, back_buffer);
    }

    std::shared_ptr<mg::Buffer> last_rendered()
    {
        return front_buffer;
    }

private:
    int compute_update_value()
    {
        float const update_ratio{3.90625}; //this will give an update of 256 in 1s  
        auto current_tick = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_tick - last_tick).count();
        float update_value = elapsed_ms / update_ratio;
        last_tick = current_tick;
        return static_cast<int>(update_value);
    }

    std::shared_ptr<mg::Buffer> front_buffer;
    std::shared_ptr<mg::Buffer> back_buffer;
    unsigned int color;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick;
    PixelBufferABGR pixel_buffer;
};

class DemoRenderable : public mg::Renderable
{
public:
    DemoRenderable(std::shared_ptr<DemoOverlayClient> const& client, geom::Rectangle rect)
        : client(client),
          position(rect)
    {
    }

    ID id() const override
    {
        return this;
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return client->last_rendered();
    }

    geom::Rectangle screen_position() const override
    {
        return position;
    }

    float alpha() const override
    {
        return 1.0f;
    }

    glm::mat4 transformation() const override
    {
        return trans;
    }

    bool shaped() const override
    {
        return false;
    }

private:
    std::shared_ptr<DemoOverlayClient> const client;
    geom::Rectangle const position;
    glm::mat4 const trans;
};

void render_loop(mir::graphics::Display& display, mir::graphics::GraphicBufferAllocator& allocator)
{
    /* Set up graceful exit on SIGINT and SIGTERM */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    mg::BufferProperties buffer_properties{
        geom::Size{512, 512},
        mir_pixel_format_abgr_8888,
        mg::BufferUsage::hardware
    };

    auto client1 = std::make_shared<DemoOverlayClient>(allocator, buffer_properties, 0xFF0000FF);
    auto client2 = std::make_shared<DemoOverlayClient>(allocator, buffer_properties, 0xFFFFFF00);

    mg::RenderableList renderlist
    {
        std::make_shared<DemoRenderable>(client1, geom::Rectangle{{0,0} , buffer_properties.size}),
        std::make_shared<DemoRenderable>(client2, geom::Rectangle{{80,80} , buffer_properties.size})
    };

    while (running)
    {
        client1->update_green_channel();
        client2->update_green_channel();
        display.for_each_display_sync_group([&](mg::DisplaySyncGroup& group)
        {
            group.for_each_display_buffer([&](mg::DisplayBuffer& buffer)
            {
                // TODO: Is make_current() really needed here?
                me::as_render_target(buffer)->make_current();
                buffer.post_renderables_if_optimizable(renderlist);
            });
            group.post();
        });
    }
}
struct GLConfig : mg::GLConfig
{
    int depth_buffer_bits() const override { return 0; }
    int stencil_buffer_bits() const override { return 0; }
};

struct DisplayReport : mg::DisplayReport 
{
    void report_successful_setup_of_native_resources() override {}
    void report_successful_egl_make_current_on_construction() override {}
    void report_successful_egl_buffer_swap_on_construction() override {}
    void report_successful_display_construction() override {}
    void report_egl_configuration(EGLDisplay, EGLConfig) override {}
    void report_vsync(unsigned int) override {}
    void report_successful_drm_mode_set_crtc_on_construction() override {}
    void report_drm_master_failure(int) override {}
    void report_vt_switch_away_failure() override {}
    void report_vt_switch_back_failure() override {}
};
}

int main(int argc, char const** argv)
try
{
    mir::logging::NullSharedLibraryProberReport null_report;
    auto config = std::make_unique<mo::DefaultConfiguration>(argc, argv);
    auto options = static_cast<mo::Configuration*>(config.get())->the_options();
    auto const& path = options->get<std::string>(mo::platform_path);
    auto platforms = mir::libraries_for_path(path, null_report);
    if (platforms.empty())
        throw std::runtime_error("no platform modules detected");
    auto platform_library = mg::module_for_device(
        platforms, dynamic_cast<mir::options::ProgramOption&>(*options));

    auto describe_fn = platform_library->load_function<mg::DescribeModule>(
        "describe_graphics_module", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    auto description = describe_fn();
    std::cout << "Loaded module: " << description->file << "\n\t"
        << "module name: " << description->name
        << "version: " << description->major_version << "."
        << description->minor_version << "."
        << description->micro_version << std::endl;

    auto platform_fn = platform_library->load_function<mg::CreateHostPlatform>(
        "create_host_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    auto platform = platform_fn(options, nullptr, std::make_shared<DisplayReport>());

    //Strange issues going on here with dlopen() + hybris (which uses gnu_indirect_functions)
    //https://github.com/libhybris/libhybris/issues/315
    //calling a GLES function here makes everything resolve correctly.
    glGetString(GL_EXTENSIONS);
    auto allocator = platform->create_buffer_allocator();
    auto display = platform->create_display(nullptr, std::make_shared<GLConfig>());

    render_loop(*display, *allocator);
    return EXIT_SUCCESS;
}
catch (std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (...)
{
    std::cerr << "unknown exception" << std::endl;
    return EXIT_FAILURE;
}
