/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include "mir/raii.h"

#include <boost/program_options.hpp>

#include <getopt.h>
#include <csignal>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <fstream>
#include <sstream>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <future>
#include <vector>
#include <utility>
#include <chrono>

namespace po = boost::program_options;

namespace
{

volatile sig_atomic_t running = 1;

/* On devices with android based openGL drivers, the vendor dispatcher table
 * may be optimized if USE_FAST_TLS_KEY is set, which hardcodes a TLS slot where
 * the vendor opengl function pointers live. Since glibc is not aware of this
 * use, collisions may happen.
 * Allocating a thread_local array helps avoid collisions by any thread_local usage
 * in async/future implementations.
 */
thread_local int dummy_tls[2];

void shutdown(int)
{
    running = 0;
}

void read_pixels(GLenum format, mir::geometry::Size const& size, void* buffer)
{
    auto width = size.width.as_uint32_t();
    auto height = size.height.as_uint32_t();

    glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);
}


uint32_t get_first_valid_output_id(MirDisplayConfiguration const& display_config)
{
    for (unsigned int i = 0; i < display_config.num_outputs; ++i)
    {
        MirDisplayOutput const& output = display_config.outputs[i];

        if (output.connected && output.used &&
            output.current_mode < output.num_modes)
        {
            return output.output_id;
        }
    }

    throw std::runtime_error("Couldn't find a valid output to screencast");
}

MirRectangle get_screen_region_from(MirDisplayConfiguration const& display_config, uint32_t output_id)
{
    if (output_id == mir_display_output_id_invalid)
        output_id = get_first_valid_output_id(display_config);

    for (unsigned int i = 0; i < display_config.num_outputs; ++i)
    {
        MirDisplayOutput const& output = display_config.outputs[i];

        if (output.output_id == output_id &&
            output.current_mode < output.num_modes)
        {
            MirDisplayMode const& mode = output.modes[output.current_mode];
            return MirRectangle{output.position_x, output.position_y,
                                mode.horizontal_resolution,
                                mode.vertical_resolution};
        }
    }

    throw std::runtime_error("Couldn't get screen region of specified output");
}

MirScreencastParameters get_screencast_params(MirConnection* connection,
                                              MirDisplayConfiguration const& display_config,
                                              std::vector<int> const& requested_size,
                                              std::vector<int> const& requested_region,
                                              uint32_t output_id)
{
    MirScreencastParameters params;
    if (requested_region.size() == 4)
    {
        /* TODO: the region should probably be validated
         * to make sure it overlaps any one of the active display areas
         */
        params.region.left = requested_region[0];
        params.region.top = requested_region[1];
        params.region.width = requested_region[2];
        params.region.height = requested_region[3];
    }
    else
    {
        params.region = get_screen_region_from(display_config, output_id);
    }

    if (requested_size.size() == 2)
    {
        params.width = requested_size[0];
        params.height = requested_size[1];
    }
    else
    {
        params.width = params.region.width;
        params.height = params.region.height;
    }

    unsigned int num_valid_formats = 0;
    mir_connection_get_available_surface_formats(connection, &params.pixel_format, 1, &num_valid_formats);

    if (num_valid_formats == 0)
        throw std::runtime_error("Couldn't find a valid pixel format for connection");

    return params;
}

double get_capture_rate_limit(MirDisplayConfiguration const& display_config, MirScreencastParameters const& params)
{
    mir::geometry::Rectangle screencast_area{
        {params.region.left, params.region.top},
        {params.region.width, params.region.height}};

    double highest_refresh_rate = 0.0;
    for (unsigned int i = 0; i < display_config.num_outputs; ++i)
    {
        MirDisplayOutput const& output = display_config.outputs[i];
        if (output.connected && output.used &&
            output.current_mode < output.num_modes)
        {
            MirDisplayMode const& mode = output.modes[output.current_mode];
            mir::geometry::Rectangle display_area{
                {output.position_x, output.position_y},
                {mode.horizontal_resolution, mode.vertical_resolution}};
            if (display_area.overlaps(screencast_area) && mode.refresh_rate > highest_refresh_rate)
                highest_refresh_rate = mode.refresh_rate;
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

struct EGLSetup
{
    EGLSetup(MirConnection* connection, MirScreencast* screencast)
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

        auto native_display =
            reinterpret_cast<EGLNativeDisplayType>(
                mir_connection_get_egl_native_display(connection));

        auto native_window =
            reinterpret_cast<EGLNativeWindowType>(
                mir_screencast_egl_native_window(screencast));

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
    }

    ~EGLSetup()
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(egl_display, egl_surface);
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
    }

    void swap_buffers() const
    {
        if (eglSwapBuffers(egl_display, egl_surface) != EGL_TRUE)
            throw std::runtime_error("Failed to swap screencast surface buffers");
    }

    GLenum pixel_read_format() const
    {
        return read_pixel_format;
    }

    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    GLenum read_pixel_format;
};

void do_screencast(EGLSetup const& egl_setup,
                   mir::geometry::Size const& size,
                   int32_t number_of_captures,
                   double capture_fps,
                   std::ostream& out_stream)
{
    static int const rgba_pixel_size{4};
    auto const frame_size_bytes = rgba_pixel_size *
                                  size.width.as_uint32_t() *
                                  size.height.as_uint32_t();

    std::vector<char> frame_data(frame_size_bytes, 0);
    auto format = egl_setup.pixel_read_format();

    auto capture_period = std::chrono::duration<double>(1.0/capture_fps);

    while (running && (number_of_captures != 0))
    {
        auto time_point = std::chrono::steady_clock::now() + capture_period;

        read_pixels(format, size, frame_data.data());

        auto write_out_future = std::async(
                std::launch::async,
                [&out_stream, &frame_data] {
                    out_stream.write(frame_data.data(), frame_data.size());
                });

        egl_setup.swap_buffers();
        write_out_future.wait();

        if (number_of_captures > 0)
            number_of_captures--;

        std::this_thread::sleep_until(time_point);
    }
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

    //avoid unused warning/error
    dummy_tls[0] = 0;

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
        mir_connection_create_display_config(connection.get()),
        &mir_display_config_destroy);

    if (display_config == nullptr)
        throw std::runtime_error("Failed to get display configuration\n");

    MirScreencastParameters params =
        get_screencast_params(connection.get(), *display_config, requested_size, screen_region, output_id);

    double capture_rate_limit = get_capture_rate_limit(*display_config, params);
    double capture_fps = capture_rate_limit/capture_interval;

    auto const screencast = mir::raii::deleter_for(
        mir_connection_create_screencast_sync(connection.get(), &params),
        [](MirScreencast* s) { if (s) mir_screencast_release_sync(s); });

    if (screencast == nullptr)
        throw std::runtime_error("Failed to create screencast");

    EGLSetup egl_setup{connection.get(), screencast.get()};
    mir::geometry::Size screencast_size {params.width, params.height};

    if (output_filename.empty() && !use_std_out)
    {
        std::stringstream ss;
        ss << "/tmp/mir_screencast_" ;
        ss << screencast_size.width << "x" << screencast_size.height;
        ss << "_" << capture_fps << "Hz";
        ss << (egl_setup.pixel_read_format() == GL_BGRA_EXT ? ".bgra" : ".rgba");
        output_filename = ss.str();
    }

    if (query_params_only)
    {
       std::cout << "Colorspace: " <<
           (egl_setup.pixel_read_format() == GL_BGRA_EXT ? "BGRA" : "RGBA") << std::endl;
       std::cout << "Output size: " <<
           screencast_size.width << "x" << screencast_size.height << std::endl;
       std::cout << "Capture rate (Hz): " << capture_fps << std::endl;
       std::cout << "Output to: " <<
           (use_std_out ? "standard out" : output_filename) << std::endl;
       return EXIT_SUCCESS;
    }

    if (use_std_out)
    {
        do_screencast(egl_setup, screencast_size, number_of_captures, capture_fps, std::cout);
    }
    else
    {
        std::ofstream file_stream(output_filename);
        do_screencast(egl_setup, screencast_size, number_of_captures, capture_fps, file_stream);
    }

    return EXIT_SUCCESS;
}
catch(std::exception const& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
