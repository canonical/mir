/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/options/program_option.h"
#include "mir/shared_library.h"
#include "mir/renderer/gl/context.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/renderer.h"
#include "mir/main_loop.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/udev/wrapper.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/default_server_configuration.h"
#include "mir/emergency_cleanup.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <boost/exception/diagnostic_information.hpp>
#include <chrono>
#include <iostream>
#include <mir/geometry/displacement.h>
#include <thread>
#include <unistd.h>

namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;

namespace
{
//TODO: With a little bit of reworking of the CMake files it would be possible to
//just depend on the relevant object libraries and not pull in the whole DefaultServerConfiguration
class MinimalServerEnvironment : private mir::DefaultServerConfiguration
{
public:
    MinimalServerEnvironment() : DefaultServerConfiguration(1, argv)
    {
    }

    ~MinimalServerEnvironment()
    {
        if (main_loop_thread.joinable())
        {
            the_main_loop()->stop();
            main_loop_thread.join();
        }
    }

    auto options() const -> std::shared_ptr<mir::options::Option>
    {
        return std::make_shared<mir::options::ProgramOption>();
    }

    auto console_services() -> std::shared_ptr<mir::ConsoleServices>
    {
        // The ConsoleServices may require a running mainloop.
        if (!main_loop_thread.joinable())
            main_loop_thread = std::thread{[this]() { the_main_loop()->run(); }};

        return the_console_services();
    }

    auto display_report() -> std::shared_ptr<mir::graphics::DisplayReport>
    {
        return the_display_report();
    }

    auto logger() -> std::shared_ptr<mir::logging::Logger>
    {
        return the_logger();
    }

    auto emergency_cleanup_registry() -> std::shared_ptr<mir::EmergencyCleanupRegistry>
    {
        return the_emergency_cleanup();
    }

    auto initial_display_configuration() -> std::shared_ptr<mir::graphics::DisplayConfigurationPolicy>
    {
        return the_display_configuration_policy();
    }

    auto gl_config() -> std::shared_ptr<mir::graphics::GLConfig>
    {
        return the_gl_config();
    }

private:
    std::thread main_loop_thread;
    static char const* argv[];
};
char const* MinimalServerEnvironment::argv[] = {"graphics_platform_test_harness", nullptr};

std::string describe_probe_result(mg::probe::Result priority)
{
    if (priority == mg::probe::unsupported)
    {
        return "UNSUPPORTED";
    }
    else if (priority == mg::probe::dummy)
    {
        return "DUMMY";
    }
    else if (priority < mg::probe::supported)
    {
        return std::string{"SUPPORTED - "} + std::to_string(mg::probe::supported - priority);
    }
    else if (priority == mg::probe::supported)
    {
        return "SUPPORTED";
    }
    else if (priority < mg::probe::best)
    {
        return std::string{"SUPPORTED + "} + std::to_string(priority - mg::probe::supported);
    }
    else if (priority == mg::probe::best)
    {
        return "BEST";
    }
    return std::string{"BEST + "} + std::to_string(priority - mg::probe::best);
}

auto test_probe(mir::SharedLibrary const& dso, MinimalServerEnvironment& config) -> std::vector<mg::SupportedDevice>
{
    try
    {
        auto probe_fn =
            dso.load_function<mg::PlatformProbe>("probe_graphics_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

        auto result = probe_fn(
            config.console_services(),
            std::make_shared<mir::udev::Context>(),
            *std::dynamic_pointer_cast<mir::options::ProgramOption>(config.options()));

        for (auto const& device : result)
        {
            std::cout << "Probe result: " << describe_probe_result(device.support_level) << "(" << device.support_level << ")" << std::endl;
        }

        return result;
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failure in probe()"});
    }
}

auto test_platform_construction(mir::SharedLibrary const& dso, std::vector<mg::SupportedDevice> const& devices, MinimalServerEnvironment& env)
    -> std::shared_ptr<mir::graphics::DisplayPlatform>
{
    try
    {
        auto create_display_platform = dso.load_function<mg::CreateDisplayPlatform>(
            "create_display_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

        for (auto const& device : devices)
        {
            if (device.support_level >= mg::probe::supported)
            {
                auto display = create_display_platform(
                    device,
                    env.options(),
                    env.emergency_cleanup_registry(),
                    env.console_services(),
                    env.display_report());

                std::cout << "Successfully constructed DisplayPlatform" << std::endl;

                return display;                
            }
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{"Device probe didn't return any supported devices"}));
        
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failure constructing platform"});
    }
}

auto test_display_construction(mir::graphics::DisplayPlatform& platform, MinimalServerEnvironment& env)
    -> std::shared_ptr<mir::graphics::Display>
{
    try
    {
        auto display = platform.create_display(
            env.initial_display_configuration(),
            env.gl_config());

        std::cout << "Successfully created display" << std::endl;

        return display;
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failure constructing Display"});
    }
}

void for_each_display_buffer(mg::Display& display, std::function<void(mg::DisplaySink&)> functor)
{
    display.for_each_display_sync_group(
        [db_functor = std::move(functor)](mg::DisplaySyncGroup& sync_group)
        {
            sync_group.for_each_display_sink(db_functor);
        });
}

auto test_display_has_at_least_one_enabled_output(mg::Display& display) -> bool
{
    int output_count{0};
    for_each_display_buffer(display, [&output_count](auto&) { ++output_count; });
    if (output_count > 0)
    {
        std::cout << "Display has " << output_count << " enabled outputs" << std::endl;
    }
    else
    {
        std::cout << "Display has no enabled outputs!" << std::endl;
    }

    return output_count > 0;
}

/*
auto hex_to_gl(unsigned char colour) -> GLclampf
{
    return static_cast<float>(colour) / 255;
}

void basic_display_swapping(mg::Display& display)
{
    for_each_display_sink(
        display,
        [](mg::DisplayBuffer& db)
        {
            auto& gl_buffer = dynamic_cast<mir::renderer::gl::RenderTarget&>(db);
            gl_buffer.make_current();
            
            for (int i = 0; i < 3; ++i)
            {
                glClearColor(
                    hex_to_gl(0xe9),
                    hex_to_gl(0x54),
                    hex_to_gl(0x20),
                    0.0f);

                glClear(GL_COLOR_BUFFER_BIT);

                gl_buffer.swap_buffers();

                sleep(1);

                glClearColor(
                    hex_to_gl(0x77),
                    hex_to_gl(0x21),
                    hex_to_gl(0x6f),
                    1.0f);

                glClear(GL_COLOR_BUFFER_BIT);

                gl_buffer.swap_buffers();

                sleep(1);
            }
        });
}
*/
std::shared_ptr<mg::Buffer> alloc_and_fill_sw_buffer(
    mg::GraphicBufferAllocator& allocator,
    mir::geometry::Size size,
    MirPixelFormat format,
    uint32_t fill_value)
{
    auto const num_px = size.width.as_uint32_t() * size.height.as_uint32_t();
    auto buffer = std::make_unique<unsigned char[]>(num_px* MIR_BYTES_PER_PIXEL(format));
    std::fill(
        reinterpret_cast<uint32_t*>(buffer.get()),
        reinterpret_cast<uint32_t*>(buffer.get() + num_px),
        fill_value);

    return mrs::alloc_buffer_with_content(
        allocator,
        buffer.get(),
        size,
        mir::geometry::Stride(size.width.as_uint32_t() * MIR_BYTES_PER_PIXEL(format)),
        format);
}

template<typename Period, typename Rep>
auto bounce_within(
    mir::geometry::Rectangle const& surface,
    mir::geometry::Displacement const& motion_per_ms,
    std::chrono::duration<Rep, Period> const& duration,
    mir::geometry::Rectangle const& bounding_box)
    -> mir::geometry::Displacement
{
    auto bounced_motion = motion_per_ms;
    auto const projected_motion =
        motion_per_ms * std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    if ((surface.bottom_right() + projected_motion).x > bounding_box.right())
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx * -1,
                bounced_motion.dy};
    }
    if ((surface.bottom_right() + projected_motion).y > bounding_box.bottom())
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx,
                bounced_motion.dy * -1};
    }
    if ((surface.bottom_right() + projected_motion).x.as_int() < 0)
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx * -1,
                bounced_motion.dy};
    }
    if ((surface.bottom_right() + projected_motion).y.as_int() < 0)
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx,
                bounced_motion.dy * -1};
    }

    return bounced_motion;
}

[[maybe_unused]]
void basic_software_buffer_drawing(
    mg::Display& display,
    std::shared_ptr<mg::RenderingPlatform> platform,
    mg::GraphicBufferAllocator& allocator,
    mir::renderer::RendererFactory& /*factory*/)
{
    uint32_t const transparent_orange = 0x002054e9;
    uint32_t const translucent_aubergine = 0xaa77216f;

    auto orange =
        alloc_and_fill_sw_buffer(allocator, {640, 480}, mir_pixel_format_xbgr_8888, transparent_orange);
    auto aubergine =
        alloc_and_fill_sw_buffer(allocator, {800, 600}, mir_pixel_format_argb_8888, translucent_aubergine);

    std::vector<std::unique_ptr<mir::renderer::Renderer>> renderers;

    int min_height{std::numeric_limits<int>::max()}, min_width{std::numeric_limits<int>::max()};
    for_each_display_buffer(
        display,
        [platform, /*&renderers, &factory,*/ &min_height, &min_width](mg::DisplaySink& sink)
        {
            if (auto gl_provider = mg::RenderingPlatform::acquire_provider<mg::GLRenderingProvider>(platform))
            {
//                auto output_surface = gl_provider->surface_for_sink(sink, );
//                renderers.push_back(factory.create_renderer_for(std::move(output_surface), gl_provider));
                min_height = std::min(min_height, sink.view_area().bottom().as_int());
                min_width = std::min(min_width, sink.view_area().right().as_int());
            }
            else
            {
                BOOST_THROW_EXCEPTION((std::runtime_error{"Platform does not support GL"}));
            }
        });

    class DummyRenderable : public mg::Renderable
    {
    public:
        DummyRenderable(
            std::shared_ptr<mg::Buffer> buffer,
            mir::geometry::Point top_left)
            : buffer_{std::move(buffer)},
              top_left{top_left}
        {
        }

        auto id() const -> mg::Renderable::ID override
        {
            return buffer_.get();
        }

        auto buffer() const -> std::shared_ptr<mg::Buffer> override
        {
            return buffer_;
        }

        auto screen_position() const -> mir::geometry::Rectangle override
        {
            return mir::geometry::Rectangle{top_left, buffer()->size()};
        }

        auto src_bounds() const -> mir::geometry::RectangleD override
        {
            return {{0, 0}, buffer()->size()};
        }

        auto alpha() const -> float override
        {
            return 1.0f;
        }

        glm::mat4 transformation() const override
        {
            return glm::mat4(1);
        }

        bool shaped() const override
        {
            return mg::contains_alpha(buffer_->pixel_format());
        }

        auto clip_area() const -> std::optional<mir::geometry::Rectangle> override
        {
            return std::optional<mir::geometry::Rectangle>{};
        }

        void set_position(mir::geometry::Point top_left)
        {
            this->top_left = top_left;
        }

        auto surface_if_any() const
            -> std::optional<mir::scene::Surface const*> override
        {
            return std::nullopt;
        }
    private:
        std::shared_ptr<mg::Buffer> const buffer_;
        mir::geometry::Point top_left;
    };

    auto orange_rectangle =
        std::make_shared<DummyRenderable>(std::move(orange), mir::geometry::Point{0, 0});
    auto aubergine_rectangle =
        std::make_shared<DummyRenderable>(std::move(aubergine), mir::geometry::Point{600, 600});

    using namespace std::literals::chrono_literals;
    using namespace std::chrono;

    auto const end_time = steady_clock::now() + 10s;

    auto last_frame_time = steady_clock::now();
    auto orange_displacement = mir::geometry::Displacement{1, 1};
    auto aubergine_displacement = mir::geometry::Displacement{-1, -1};
    auto const bounding_box = mir::geometry::Rectangle{{0, 0,}, {min_width, min_height}};

    while (steady_clock::now() < end_time)
    {
        auto const delta_t = steady_clock::now() - last_frame_time;
        last_frame_time = steady_clock::now();

        auto const delta_t_ms = duration_cast<milliseconds>(delta_t).count();

        orange_displacement = bounce_within(
            orange_rectangle->screen_position(),
            orange_displacement,
            delta_t,
            bounding_box);
        orange_rectangle->set_position(
            orange_rectangle->screen_position().top_left +
            delta_t_ms * orange_displacement);

        aubergine_displacement = bounce_within(
            aubergine_rectangle->screen_position(),
            aubergine_displacement,
            delta_t,
            bounding_box);
        aubergine_rectangle->set_position(
            aubergine_rectangle->screen_position().top_left +
            delta_t_ms * aubergine_displacement);

        for(auto const& renderer : renderers)
        {
            renderer->render({orange_rectangle, aubergine_rectangle});
        }
    }
}

void print_diagnostic_information(std::exception const& err)
{
    std::cout << "Error: " << err.what() << std::endl;
    std::cout << boost::diagnostic_information(err) << std::endl;
    try
    {
        std::rethrow_if_nested(err);
    }
    catch (std::exception const& inner)
    {
        std::cout << "Caused by: " << std::endl;
        print_diagnostic_information(inner);
    }
    catch (...)
    {
        std::cout << "Caused by broken exception!" << std::endl;
    }
}
}

int main(int argc, char const** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " PLATFORM_DSO" << std::endl;
        return -1;
    }

    mir::SharedLibrary platform_dso{argv[1]};

    MinimalServerEnvironment config;

    bool success = true;
    try
    {
        auto devices = test_probe(platform_dso, config);
        if (auto platform = test_platform_construction(platform_dso, devices, config))
        {
            if (auto display = test_display_construction(*platform, config))
            {
                success &= test_display_has_at_least_one_enabled_output(*display);
//                success &= test_display_buffers_support_gl(*display);
//                basic_display_swapping(*display);

                // TODO: Update for display/render platform split
//                auto buffer_allocator = platform->create_buffer_allocator(*display);
//
//                basic_software_buffer_drawing(
//                    *display,
//                    *buffer_allocator,
//                    *config.render_factory());
            }
            else
            {
                success = false;
            }
        }
        else
        {
            success = false;
        }
    }
    catch (std::exception const& e)
    {
        print_diagnostic_information(e);
        success = false;
    }
    catch (...)
    {
        std::cout << "Error: Broken exception" << std::endl;
        success = false;
    }
    std::cout << (success ? "PASS" : "FAILURE") << std::endl;
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
