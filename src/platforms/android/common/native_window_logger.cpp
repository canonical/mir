/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "native_window_logger.h"
#include "mir/raii.h"
#include <iostream>

#define CASE_KEY(var) case var: return out << #var;
namespace mga = mir::graphics::android;

namespace
{

struct NativePerformKey
{
    int key;
};

struct NativeQueryKey
{
    int key;
};

std::ostream& operator<<(std::ostream& out, NativeQueryKey key)
{
    switch (key.key)
    {
        CASE_KEY(NATIVE_WINDOW_WIDTH)
        CASE_KEY(NATIVE_WINDOW_HEIGHT)
        CASE_KEY(NATIVE_WINDOW_FORMAT)
        CASE_KEY(NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS)
        CASE_KEY(NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER)
        CASE_KEY(NATIVE_WINDOW_CONCRETE_TYPE)
        CASE_KEY(NATIVE_WINDOW_DEFAULT_WIDTH)
        CASE_KEY(NATIVE_WINDOW_DEFAULT_HEIGHT)
        CASE_KEY(NATIVE_WINDOW_TRANSFORM_HINT)
        CASE_KEY(NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND)
        CASE_KEY(NATIVE_WINDOW_CONSUMER_USAGE_BITS)
        CASE_KEY(NATIVE_WINDOW_STICKY_TRANSFORM)
        CASE_KEY(NATIVE_WINDOW_DEFAULT_DATASPACE)
        CASE_KEY(NATIVE_WINDOW_BUFFER_AGE)
        default: return out << "unknown query key: " << key.key;
    }
}

std::ostream& operator<<(std::ostream& out, NativePerformKey key)
{
    switch (key.key)
    {
        CASE_KEY(NATIVE_WINDOW_SET_USAGE)
        CASE_KEY(NATIVE_WINDOW_CONNECT)
        CASE_KEY(NATIVE_WINDOW_DISCONNECT)
        CASE_KEY(NATIVE_WINDOW_SET_CROP)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFER_COUNT)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_GEOMETRY)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_TRANSFORM)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_FORMAT)
        CASE_KEY(NATIVE_WINDOW_SET_SCALING_MODE)
        CASE_KEY(NATIVE_WINDOW_LOCK)
        CASE_KEY(NATIVE_WINDOW_UNLOCK_AND_POST)
        CASE_KEY(NATIVE_WINDOW_API_CONNECT)
        CASE_KEY(NATIVE_WINDOW_API_DISCONNECT)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS)
        CASE_KEY(NATIVE_WINDOW_SET_POST_TRANSFORM_CROP)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_STICKY_TRANSFORM)
        CASE_KEY(NATIVE_WINDOW_SET_SIDEBAND_STREAM)
        CASE_KEY(NATIVE_WINDOW_SET_BUFFERS_DATASPACE)
        CASE_KEY(NATIVE_WINDOW_SET_SURFACE_DAMAGE)
        default: return out << "unknown perform key: " << key.key;
    }
}

std::ostream& operator<<(std::ostream& out, mga::BufferEvent type)
{
    switch (type)
    {
        case mga::BufferEvent::Queue: return out << "queueBuffer";
        case mga::BufferEvent::Dequeue: return out << "dequeueBuffer";
        case mga::BufferEvent::Cancel: return out << "cancelBuffer";
        default: return out;
    }
}

std::ostream& operator<<(std::ostream& out, ANativeWindow const* window)
{
    return out << "[EGLNativeWindow] addr (" << static_cast<void const*>(window) << "): "; 
}
}

void mga::ConsoleNativeWindowLogger::buffer_event(
    BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf, int fence) const
{
    std::cout << win << type << ": " << buf << ", fence: ";
    if ( fence > 0 )
        std::cout << fence;
    else
        std::cout << "none";

    std::cout << std::endl;
}

void mga::ConsoleNativeWindowLogger::buffer_event(
    BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf) const
{
    std::cout << win << type << "_deprecated: " << buf << std::endl;
}

void mga::ConsoleNativeWindowLogger::query_event(ANativeWindow const* win, int type, int result) const
{
    auto hex = mir::raii::paired_calls([] { std::cout << std::hex; }, [] { std::cout << std::dec; });
    std::cout << win << "query: " << NativeQueryKey{type} << ": result: 0x" <<
        result << std::endl;
}

void mga::ConsoleNativeWindowLogger::perform_event(
    ANativeWindow const* win, int type, std::vector<int> const& args) const
{
    auto hex = mir::raii::paired_calls([] { std::cout << std::hex; }, [] { std::cout << std::dec; });
    std::cout << win << "perform: " << NativePerformKey{type} << ": ";
    for (auto i = 0u; i < args.size(); i++)
    {
        if (i != 0u)
            std::cout << ", ";
        std::cout <<  "0x" << args[i];
    }
    std::cout << std::endl;;
}

void mga::NullNativeWindowLogger::buffer_event(mga::BufferEvent, ANativeWindow const*, ANativeWindowBuffer*, int) const
{
}

void mga::NullNativeWindowLogger::buffer_event(mga::BufferEvent, ANativeWindow const*, ANativeWindowBuffer*) const
{
}

void mga::NullNativeWindowLogger::query_event(ANativeWindow const*, int, int) const
{
}

void mga::NullNativeWindowLogger::perform_event(ANativeWindow const*, int, std::vector<int> const&) const
{
}
