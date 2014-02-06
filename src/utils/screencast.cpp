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

#include "mir/raii.h"

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

namespace
{

volatile sig_atomic_t running = 1;

void shutdown(int)
{
    running = 0;
}

std::future<void> write_frame_to_file(
    std::vector<char> const& frame_data, int frame_number, GLenum format)
{
    return std::async(
        std::launch::async,
        [&frame_data, frame_number, format]
        {
            std::stringstream ss;
            ss << "/tmp/mir_" ;
            ss.width(5);
            ss.fill('0');
            ss << frame_number;
            ss << (format == GL_BGRA_EXT ? ".bgra" : ".rgba");
            std::ofstream f(ss.str());
            f.write(frame_data.data(), frame_data.size());
        });
}

GLenum read_pixels(mir::geometry::Size const& size, void* buffer)
{
    auto width = size.width.as_uint32_t();
    auto height = size.height.as_uint32_t();

    GLenum format = GL_BGRA_EXT;

    glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);

    if (glGetError() != GL_NO_ERROR)
    {
        format = GL_RGBA;
        glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);
    }

    return format;
}


uint32_t get_first_valid_output_id(MirConnection* connection)
{
    auto const conf = mir::raii::deleter_for(
        mir_connection_create_display_config(connection),
        &mir_display_config_destroy);

    if (conf == nullptr)
        throw std::runtime_error("Failed to get display configuration\n");

    for (unsigned i = 0; i < conf->num_outputs; ++i)
    {
        MirDisplayOutput const& output = conf->outputs[i];

        if (output.connected && output.used &&
            output.current_mode < output.num_modes)
        {
            return output.output_id;
        }
    }

    throw std::runtime_error("Couldn't find a valid output to screencast");
}

mir::geometry::Size get_output_size(MirConnection* connection, uint32_t output_id)
{
    auto const conf = mir::raii::deleter_for(
        mir_connection_create_display_config(connection),
        &mir_display_config_destroy);

    if (conf == nullptr)
        throw std::runtime_error("Failed to get display configuration\n");

    for (unsigned i = 0; i < conf->num_outputs; ++i)
    {
        MirDisplayOutput const& output = conf->outputs[i];

        if (output.output_id == output_id &&
            output.current_mode < output.num_modes)
        {
            MirDisplayMode const& mode = output.modes[output.current_mode];
            return mir::geometry::Size{mode.horizontal_resolution, mode.vertical_resolution};
        }
    }

    throw std::runtime_error("Couldn't get size of specified output");
}

void print_usage()
{
    std::cout << "Usage " << std::endl
              << "    -m <Mir server socket>" << std::endl
              << "    -o <Output id>" << std::endl
              << "    -h: this help text" << std::endl;
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
    }

    ~EGLSetup()
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(egl_display, egl_surface);
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
    }

    void swap_buffers()
    {
        if (eglSwapBuffers(egl_display, egl_surface) != EGL_TRUE)
            throw std::runtime_error("Failed to swap screencast surface buffers");
    }

    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
};

void do_screencast(MirConnection* connection, MirScreencast* screencast,
                   uint32_t output_id)
{
    static int const rgba_pixel_size{4};

    auto const frame_size = get_output_size(connection, output_id);
    auto const frame_size_bytes = rgba_pixel_size *
                                  frame_size.width.as_uint32_t() *
                                  frame_size.height.as_uint32_t();

    int frame_number{0};
    std::vector<char> frame_data(frame_size_bytes, 0);
    std::future<void> frame_written_future =
        std::async(std::launch::deferred, []{});

    EGLSetup egl_setup{connection, screencast};

    while (running)
    {
        frame_written_future.wait();

        auto format = read_pixels(frame_size, frame_data.data());
        frame_written_future = write_frame_to_file(frame_data, frame_number, format);

        egl_setup.swap_buffers();
        ++frame_number;
    }
}

}

int main(int argc, char* argv[])
try
{
    int arg;
    opterr = 0;
    char const* socket_file = nullptr;
    uint32_t output_id = mir_display_output_id_invalid;

    while ((arg = getopt (argc, argv, "hm:o:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;

        case 'o':
            output_id = std::stoul(optarg);
            break;

        case '?':
        case 'h':
            print_usage();
            return EXIT_SUCCESS;

        default:
            print_usage();
            return EXIT_FAILURE;
        }
    }

    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);

    auto const connection = mir::raii::deleter_for(
        mir_connect_sync(socket_file, "mirscreencast"),
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

    if (output_id == mir_display_output_id_invalid)
        output_id = get_first_valid_output_id(connection.get());

    MirScreencastParameters params{
        output_id, 0, 0, mir_pixel_format_invalid};

    std::cout << "Starting screencast for output id " << params.output_id << std::endl;

    auto const screencast = mir::raii::deleter_for(
        mir_connection_create_screencast_sync(connection.get(), &params),
        [](MirScreencast* s) { if (s) mir_screencast_release_sync(s); });

    if (screencast == nullptr)
        throw std::runtime_error("Failed to create screencast");

    do_screencast(connection.get(), screencast.get(), output_id);

    return EXIT_SUCCESS;
}
catch(std::exception const& e)
{
    std::cerr << e.what() << std::endl;

    return EXIT_FAILURE;
}
