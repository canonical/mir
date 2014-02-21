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

//In android, waiting for a future is causing the gl/egl context to become invalid
//possibly due to assumptions in libhybris/android linker.
//A TLS allocation in the main thread is forced with this variable which seems to push
//the gl/egl context TLS into a slot where the future wait code does not overwrite it.
thread_local int tls_hack[2];

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
              << "    -n <Number of frames to capture>" << std::endl
              << "        default (-1) is to capture infinite frames" << std::endl
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

    void swap_buffers()
    {
        if (eglSwapBuffers(egl_display, egl_surface) != EGL_TRUE)
            throw std::runtime_error("Failed to swap screencast surface buffers");
    }

    GLenum pixel_read_format()
    {
        return read_pixel_format;
    }

    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    GLenum read_pixel_format;
};

void do_screencast(MirConnection* connection, MirScreencast* screencast,
                   uint32_t output_id, int32_t number_of_captures)
{
    static int const rgba_pixel_size{4};

    auto const frame_size = get_output_size(connection, output_id);
    auto const frame_size_bytes = rgba_pixel_size *
                                  frame_size.width.as_uint32_t() *
                                  frame_size.height.as_uint32_t();

    std::vector<char> frame_data(frame_size_bytes, 0);

    EGLSetup egl_setup{connection, screencast};
    auto format = egl_setup.pixel_read_format();

    std::stringstream ss;
    ss << "/tmp/mir_screencast_" ;
    ss << frame_size.width << "x" << frame_size.height;
    ss << (format == GL_BGRA_EXT ? ".bgra" : ".rgba");
    std::ofstream video_file(ss.str());

    while (running && (number_of_captures != 0))
    {
        read_pixels(format, frame_size, frame_data.data());

        auto write_out_future = std::async(
                std::launch::async,
                [&video_file, &frame_data] {
                    video_file.write(frame_data.data(), frame_data.size());
                });

        egl_setup.swap_buffers();

        write_out_future.wait();

        if (number_of_captures > 0)
            number_of_captures--;
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
    int32_t number_of_captures = -1;

    //avoid unused warning/error
    tls_hack[0] = 0;

    while ((arg = getopt (argc, argv, "hm:o:n:")) != -1)
    {
        switch (arg)
        {
        case 'n':
            number_of_captures = std::stoi(std::string(optarg)); 
            break;
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

    do_screencast(connection.get(), screencast.get(), output_id, number_of_captures);

    return EXIT_SUCCESS;
}
catch(std::invalid_argument const& e)
{
    std::cerr << "Invalid Argument" << std::endl;
    print_usage();
    return EXIT_FAILURE;
}
catch(std::exception const& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
