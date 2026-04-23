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

#ifndef MIR_WAYLANDRS_PROTOCOL_ERROR
#define MIR_WAYLANDRS_PROTOCOL_ERROR

#include <cstdarg>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace mir
{
namespace wayland_rs
{
class ProtocolError : public std::runtime_error
{
public:
    ProtocolError(uint32_t object_id, uint32_t error_code, char const* fmt, ...);
    auto what() const noexcept -> char const* override;
    auto code() const noexcept -> uint32_t;
    auto message() const noexcept -> std::string;

private:
    uint32_t error_code;
    std::string message_;
};
}
}

#endif
