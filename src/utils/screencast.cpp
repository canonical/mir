/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"
#include "mir_toolkit/mir_buffer_stream.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"
#include "mir/raii.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <boost/program_options.hpp>

#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <future>
#include <vector>
#include <utility>
#include <chrono>
#include <csignal>

namespace po = boost::program_options;

namespace
{

std::atomic<bool> running;

struct ScreencastConfiguration
{
    uint32_t width;
    uint32_t height;
    MirRectangle region;
    MirPixelFormat pixel_format;
};

void shutdown(int)
{
    running = false;
}

uint32_t get_first_valid_output_id(MirDisplayConfig const& display_config)
{
    size_t num_outputs = mir_display_config_get_num_outputs(&display_config);
    for (unsigned int i = 0; i < num_outputs; ++i)
    {
        auto output = mir_display_config_get_output(&display_config, i);
        auto state  = mir_output_get_connection_state(output);
        size_t num_modes = mir_output_get_num_modes(output);

        if (state == mir_output_connection_state_connected &&
            mir_output_is_enabled(output) &&
            mir_output_get_current_mode_index(output) < num_modes)
        {
            return mir_output_get_id(output);
        }
    }

    throw std::runtime_error("Couldn't find a valid output to screencast");
}

MirRectangle get_screen_region_from(MirDisplayConfig const& display_config, uint32_t output_id)
{
    if (output_id == mir_display_output_id_invalid)
        output_id = get_first_valid_output_id(display_config);

    size_t num_outputs = mir_display_config_get_num_outputs(&display_config);
    for (unsigned int i = 0; i < num_outputs; ++i)
    {
        auto output = mir_display_config_get_output(&display_config, i);
        size_t num_modes = mir_output_get_num_modes(output);
        size_t id = mir_output_get_id(output);

        if (id == output_id &&
            mir_output_get_current_mode_index(output) < num_modes)
        {
            auto pos_x  = mir_output_get_position_x(output);
            auto pos_y  = mir_output_get_position_y(output);
            auto mode   = mir_output_get_current_mode(output);
            uint32_t width  = mir_output_mode_get_width(mode);
            uint32_t height = mir_output_mode_get_height(mode);
            return MirRectangle{pos_x, pos_y, width, height};
        }
    }

    throw std::runtime_error("Couldn't get screen region of specified output");
}

ScreencastConfiguration get_screencast_config(MirConnection* connection,
                                              MirDisplayConfig const& display_config,
                                              std::vector<int> const& requested_size,
                                              std::vector<int> const& requested_region,
                                              uint32_t output_id)
{
    ScreencastConfiguration config;

    if (requested_region.size() == 4)
    {
        /* TODO: the region should probably be validated
         * to make sure it overlaps any one of the active display areas
         */
        config.region.left = requested_region[0];
        config.region.top = requested_region[1];
        config.region.width = requested_region[2];
        config.region.height = requested_region[3];
    }
    else
    {
        config.region = get_screen_region_from(display_config, output_id);
    }

    if (requested_size.size() == 2)
    {
        config.width = requested_size[0];
        config.height = requested_size[1];
    }
    else
    {
        config.width  = config.region.width;
        config.height = config.region.height;
    }

    unsigned int num_valid_formats = 0;
    mir_connection_get_available_surface_formats(connection, &config.pixel_format, 1, &num_valid_formats);

    if (num_valid_formats == 0)
        throw std::runtime_error("Couldn't find a valid pixel format for connection");

    return config;
}

double get_capture_rate_limit(MirDisplayConfig const& display_config, ScreencastConfiguration const& config)
{
    mir::geometry::Rectangle screencast_area{
        {config.region.left,  config.region.top},
        {config.region.width, config.region.height}};

    double highest_refresh_rate = 0.0;
    size_t num_outputs = mir_display_config_get_num_outputs(&display_config);
    for (unsigned int i = 0; i < num_outputs; ++i)
    {
        auto output = mir_display_config_get_output(&display_config, i);
        auto state  = mir_output_get_connection_state(output);

        size_t num_modes = mir_output_get_num_modes(output);

        if (state == mir_output_connection_state_connected &&
            mir_output_is_enabled(output) &&
            mir_output_get_current_mode_index(output) < num_modes)
        {
            auto pos_x   = mir_output_get_position_x(output);
            auto pos_y   = mir_output_get_position_y(output);
            auto mode    = mir_output_get_current_mode(output);
            auto refresh = mir_output_mode_get_refresh_rate(mode);
            uint32_t width   = mir_output_mode_get_width(mode);
            uint32_t height  = mir_output_mode_get_height(mode);

            mir::geometry::Rectangle display_area{
                {pos_x, pos_y}, {width, height}};
            if (display_area.overlaps(screencast_area) && refresh > highest_refresh_rate)
                highest_refresh_rate = refresh;
        }
    }

    /* Either the display config has invalid refresh rate data
     * or the screencast region is outside of any display area, either way
     * let's default to sensible max capture limit
     */
    if (highest_refresh_rate == 0.0)
        highest_refresh_rate = 60.0;

    return highest_refresh_rate;
}

std::string mir_pixel_format_to_string(MirPixelFormat format)
{
    // Don't know of any big endian platform supported by mir
    switch(format)
    {
    case mir_pixel_format_abgr_8888:
        return "RGBA";
    case mir_pixel_format_xbgr_8888:
        return "RGBX";
    case mir_pixel_format_argb_8888:
        return "BGRA";
    case mir_pixel_format_xrgb_8888:
        return "BGRX";
    case mir_pixel_format_bgr_888:
        return "BGR";
    case mir_pixel_format_rgb_888:
        return "RGB";
    case mir_pixel_format_rgb_565:
        return "RGB565";
    case mir_pixel_format_rgba_5551:
        return "RGBA5551";
    case mir_pixel_format_rgba_4444:
        return "RGBA4444";
    default:
        throw std::logic_error("Invalid pixel format");
    }
}

std::string to_file_extension(std::string format)
{
    std::string ext{"."};
    ext.append(format);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

MirGraphicsRegion graphics_region_for(MirBufferStream* buffer_stream)
{
    MirGraphicsRegion region{0, 0, 0, mir_pixel_format_invalid, nullptr};
    mir_buffer_stream_get_graphics_region(buffer_stream, &region);

    if (region.vaddr == nullptr)
        throw std::runtime_error("Failed to obtain screencast buffer");

    return region;
}

class Screencast
{
public:
    virtual ~Screencast() = default;
    virtual std::string pixel_format() = 0;

    void run(std::ostream& stream)
    {
        while (running && (number_of_captures != 0))
        {
            auto time_point = std::chrono::steady_clock::now() + capture_period;

            capture_to(stream);

            if (number_of_captures > 0)
                number_of_captures--;

            std::this_thread::sleep_until(time_point);
        }
    }

    virtual void capture_to(std::ostream& stream) = 0;

protected:
    Screencast(int number_of_captures, double capture_fps)
        : number_of_captures{number_of_captures},
          capture_period{1.0/capture_fps}
    {
    }

private:
    int number_of_captures;
    std::chrono::duration<double> capture_period;
    Screencast(Screencast const&) = delete;
    Screencast& operator=(Screencast const&) = delete;
};

class BufferStreamScreencast : public Screencast
{
public:
    BufferStreamScreencast(int num_captures, double capture_fps,
                           ScreencastConfiguration* config,
                           MirBufferStream* buffer_stream)
        : Screencast(num_captures, capture_fps),
          buffer_stream{buffer_stream},
          pixel_format_{mir_pixel_format_to_string(config->pixel_format)}
    {
    }

    std::string pixel_format() override
    {
        return pixel_format_;
    }

    void capture_to(std::ostream& stream) override
    {
        MirGraphicsRegion const region{graphics_region_for(buffer_stream)};
        int const line_size{region.width * MIR_BYTES_PER_PIXEL(region.pixel_format)};

        // Contents are rendered up-side down, read them bottom to top
        auto addr = region.vaddr + (region.height - 1)*region.stride;
        for (int i = 0; i < region.height; i++)
        {
            stream.write(addr, line_size);
            addr -= region.stride;
        }

        mir_buffer_stream_swap_buffers_sync(buffer_stream);
    }

private:
    MirBufferStream* const buffer_stream;
    std::string const pixel_format_;
};

class EGLScreencast : public Screencast
{
public:
    EGLScreencast(int num_captures, double capture_fps,
                  MirConnection* connection, ScreencastConfiguration* config,
                  MirBufferStream* buffer_stream)
        : Screencast(num_captures, capture_fps),
          width{config->width},
          height{config->height}
    {
        static EGLint const attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE};

        static EGLint const context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE };
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto native_display =
            reinterpret_cast<EGLNativeDisplayType>(
                mir_connection_get_egl_native_display(connection));

        auto native_window =
            reinterpret_cast<EGLNativeWindowType>(
                mir_buffer_stream_get_egl_native_window(buffer_stream));
#pragma GCC diagnostic pop
        egl_display = eglGetDisplay(native_display);

        eglInitialize(egl_display, nullptr, nullptr);

        int n;
        eglChooseConfig(egl_display, attribs, &egl_config, 1, &n);

        egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
        if (egl_surface == EGL_NO_SURFACE)
            throw std::runtime_error("Failed to create EGL screencast surface");

        egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
        if (egl_context == EGL_NO_CONTEXT)
            throw std::runtime_error("Failed to create EGL context for screencast");

        if (eglMakeCurrent(egl_display, egl_surface,
                           egl_surface, egl_context) != EGL_TRUE)
        {
            throw std::runtime_error("Failed to make screencast surface current");
        }

        uint32_t a_pixel;
        glReadPixels(0, 0, 1, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, &a_pixel);
        if (glGetError() == GL_NO_ERROR)
            read_pixel_format = GL_BGRA_EXT;
        else
            read_pixel_format = GL_RGBA;

        int const rgba_pixel_size{4};
        auto const frame_size_bytes = rgba_pixel_size * width * height;
        buffer.resize(frame_size_bytes);
    }

    ~EGLScreencast()
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(egl_display, egl_surface);
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
    }

    void capture_to(std::ostream& stream) override
    {
        void* data = buffer.data();
        glReadPixels(0, 0, width, height, read_pixel_format, GL_UNSIGNED_BYTE, data);

        auto write_out_future = std::async(
            std::launch::async,
            [this, &stream] {
            stream.write(buffer.data(), buffer.size());
            });

        if (eglSwapBuffers(egl_display, egl_surface) != EGL_TRUE)
            throw std::runtime_error("Failed to swap screencast surface buffers");

        write_out_future.wait();
    }

    std::string pixel_format() override
    {
        return read_pixel_format == GL_BGRA_EXT ? "BGRA" : "RGBA";
    }

private:
    unsigned int const width;
    unsigned int const height;
    std::vector<char> buffer;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    GLenum read_pixel_format;
};

std::unique_ptr<Screencast> create_screencast(int num_captures, double capture_fps,
                                              MirConnection* connection,
                                              ScreencastConfiguration* config,
                                              MirBufferStream* buffer_stream)
{
    try
    {
        return std::make_unique<BufferStreamScreencast>(num_captures, capture_fps, config, buffer_stream);
    }
    catch(...)
    {
    }
    // Fallback to EGL if MirBufferStream can't be used directly
    return std::make_unique<EGLScreencast>(num_captures, capture_fps, connection, config, buffer_stream);
}
}

int main(int argc, char* argv[])
try
{
    uint32_t output_id = mir_display_output_id_invalid;
    int number_of_captures = -1;
    std::string output_filename;
    std::string socket_filename;
    std::vector<int> screen_region;
    std::vector<int> requested_size;
    bool use_std_out = false;
    bool query_params_only = false;
    int capture_interval = 1;

    po::options_description desc("Usage");
    desc.add_options()
        ("help,h", "displays this message")
        ("number-of-frames,n",
            po::value<int>(&number_of_captures), "number of frames to capture")
        ("display-id,d",
            po::value<uint32_t>(&output_id), "id of the display to capture")
        ("mir-socket-file,m",
            po::value<std::string>(&socket_filename), "mir server socket filename")
        ("file,f",
            po::value<std::string>(&output_filename), "output filename (default is /tmp/mir_screencast_<w>x<h>.<rgba|bgra>")
        ("size,s",
            po::value<std::vector<int>>(&requested_size)->multitoken(),
            "screencast size [width height]")
        ("screen-region,r",
            po::value<std::vector<int>>(&screen_region)->multitoken(),
            "screen region to capture [left top width height]")
        ("stdout", po::value<bool>(&use_std_out)->zero_tokens(), "use stdout for output (--file is ignored)")
        ("query",
            po::value<bool>(&query_params_only)->zero_tokens(),
            "only queries the colorspace and output size used but does not start screencast")
        ("cap-interval",
            po::value<int>(&capture_interval),
            "adjusts the capture rate to <arg> display refresh intervals\n"
            "1 -> capture at display rate\n2 -> capture at half the display rate, etc..");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("size") && requested_size.size() != 2)
            throw po::error("invalid number of parameters specified for size");

        if (vm.count("screen-region") && screen_region.size() != 4)
            throw po::error("invalid number of parameters specified for screen-region");

        if (vm.count("display-id") && vm.count("screen-region"))
            throw po::error("cannot set both display-id and screen-region");

        if (vm.count("cap-interval") && capture_interval < 1)
            throw po::error("invalid capture interval");
    }
    catch(po::error& e)
    {
        std::cerr << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    running = true;
    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);

    char const* socket_name = vm.count("mir-socket-file") ? socket_filename.c_str() : nullptr;
    auto const connection = mir::raii::deleter_for(
        mir_connect_sync(socket_name, "mirscreencast"),
        [](MirConnection* c) { if (c) mir_connection_release(c); });

    if (connection == nullptr || !mir_connection_is_valid(connection.get()))
    {
        std::string msg("Failed to connect to server.");
        if (connection)
        {
            msg += " Error was :";
            msg += mir_connection_get_error_message(connection.get());
        }
        throw std::runtime_error(std::string(msg));
    }

    auto const display_config = mir::raii::deleter_for(
        mir_connection_create_display_configuration(connection.get()),
        &mir_display_config_release);

    if (display_config == nullptr)
        throw std::runtime_error("Failed to get display configuration\n");

    MirScreencastSpec* spec = mir_create_screencast_spec(connection.get());
    auto screencast_config = get_screencast_config(connection.get(), *display_config,
                                                   requested_size, screen_region, output_id);

    mir_screencast_spec_set_width(spec, screencast_config.width);
    mir_screencast_spec_set_height(spec, screencast_config.height);
    mir_screencast_spec_set_pixel_format(spec, screencast_config.pixel_format);
    mir_screencast_spec_set_capture_region(spec, &screencast_config.region);

    double capture_rate_limit = get_capture_rate_limit(*display_config, screencast_config);
    double capture_fps = capture_rate_limit/capture_interval;

    auto const mir_screencast = mir::raii::deleter_for(
        mir_screencast_create_sync(spec),
        [](MirScreencast* s) { if (s) mir_screencast_release_sync(s); });

    mir_screencast_spec_release(spec);

    if (mir_screencast == nullptr)
        throw std::runtime_error("Failed to create screencast");

    auto buffer_stream = mir_screencast_get_buffer_stream(mir_screencast.get());
    if (buffer_stream == nullptr)
        throw std::runtime_error("Failed to obtain buffer stream from screencast");

    auto screencast = create_screencast(number_of_captures, capture_fps, connection.get(), &screencast_config, buffer_stream);

    if (output_filename.empty() && !use_std_out)
    {
        std::stringstream ss;
        ss << "/tmp/mir_screencast_" ;
        ss << screencast_config.width << "x" << screencast_config.height;
        ss << "_" << capture_fps << "Hz";
        ss << to_file_extension(screencast->pixel_format());
        output_filename = ss.str();
    }

    if (query_params_only)
    {
       std::cout << "Colorspace: " << screencast->pixel_format() << std::endl;
       std::cout << "Output size: " <<
           screencast_config.width << "x" << screencast_config.height << std::endl;
       std::cout << "Capture rate (Hz): " << capture_fps << std::endl;
       std::cout << "Output to: " <<
           (use_std_out ? "standard out" : output_filename) << std::endl;
       return EXIT_SUCCESS;
    }

    if (use_std_out)
    {
        screencast->run(std::cout);
    }
    else
    {
        std::ofstream file_stream(output_filename);
        screencast->run(file_stream);
    }

    return EXIT_SUCCESS;
}
catch(std::exception const& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
