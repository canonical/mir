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

#include "protocol_error.h"

#include <cstdio>
#include <string>
#include <vector>

namespace
{
auto format_message(uint32_t object_id, uint32_t error_code, char const* fmt, va_list args) -> std::string
{
    va_list args_copy;
    va_copy(args_copy, args);
    auto const len = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (len < 0)
    {
        return std::to_string(object_id) + ":" + std::to_string(error_code) + ": formatting error";
    }

    std::vector<char> buf(len + 1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args);

    return std::to_string(object_id) + ":" + std::to_string(error_code) + ": " + buf.data();
}
}

mir::wayland_rs::ProtocolError::ProtocolError(uint32_t object_id, uint32_t error_code, char const* fmt, ...)
    : std::runtime_error{"Client protocol error"},
      error_code(error_code)
{
    va_list args;
    va_start(args, fmt);
    message_ = format_message(object_id, error_code, fmt, args);
    va_end(args);
}

auto mir::wayland_rs::ProtocolError::what() const noexcept -> char const*
{
    return message_.c_str();
}

auto mir::wayland_rs::ProtocolError::code() const noexcept -> uint32_t
{
    return error_code;
}

auto mir::wayland_rs::ProtocolError::message() const noexcept -> std::string
{
    return message_;
}
