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

#include "mir/logging/logger.h"
#include "native_window_report.h"
#include <sstream>

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
    return out << "addr (" << static_cast<void const*>(window) << "): "; 
}
}

mga::ConsoleNativeWindowReport::ConsoleNativeWindowReport(
    std::shared_ptr<mir::logging::Logger> const& logger) :
    logger(logger)
{
}

void mga::ConsoleNativeWindowReport::buffer_event(
    BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf, int fence) const
{
    std::stringstream str;

    str << win << type << ": " << buf << ", fence: ";
    if ( fence > 0 )
        str << fence;
    else
        str << "none";

    logger->log(mir::logging::Severity::debug, str.str(), component_name); 
}

void mga::ConsoleNativeWindowReport::buffer_event(
    BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf) const
{
    std::stringstream str;
    str << win << type << "_deprecated: " << buf;
    logger->log(mir::logging::Severity::debug, str.str(), component_name); 
}

void mga::ConsoleNativeWindowReport::query_event(ANativeWindow const* win, int type, int result) const
{
    std::stringstream str;
    str << std::hex << win << "query: " << NativeQueryKey{type} << ": result: 0x" << result;
    logger->log(mir::logging::Severity::debug, str.str(), component_name); 
}

void mga::ConsoleNativeWindowReport::perform_event(
    ANativeWindow const* win, int type, std::vector<int> const& args) const
{
    std::stringstream str;
    str << std::hex << win << "perform: " << NativePerformKey{type} << ": ";
    for (auto i = 0u; i < args.size(); i++)
    {
        if (i != 0u)
            str << ", ";
        str <<  "0x" << args[i];
    }
    logger->log(mir::logging::Severity::debug, str.str(), component_name); 
}

void mga::NullNativeWindowReport::buffer_event(mga::BufferEvent, ANativeWindow const*, ANativeWindowBuffer*, int) const
{
}

void mga::NullNativeWindowReport::buffer_event(mga::BufferEvent, ANativeWindow const*, ANativeWindowBuffer*) const
{
}

void mga::NullNativeWindowReport::query_event(ANativeWindow const*, int, int) const
{
}

void mga::NullNativeWindowReport::perform_event(ANativeWindow const*, int, std::vector<int> const&) const
{
}
