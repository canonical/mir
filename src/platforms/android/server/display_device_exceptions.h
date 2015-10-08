/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_DEVICE_EXCEPTIONS_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_DEVICE_EXCEPTIONS_H_

#include <stdexcept>
#include <string>

namespace mir
{
namespace graphics
{
namespace android
{

class DisplayDisconnectedException : public std::runtime_error
{
public:
    DisplayDisconnectedException(std::string const& message) : std::runtime_error(message) {}
};

class ExternalDisplayError : public std::runtime_error
{
public:
    ExternalDisplayError(std::string const& message) : std::runtime_error(message) {}
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_DEVICE_EXCEPTIONS_H_ */
