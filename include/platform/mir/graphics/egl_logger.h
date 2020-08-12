/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_GRAPHICS_EGL_LOGGER_H_
#define MIR_PLATFORM_GRAPHICS_EGL_LOGGER_H_

#include <memory>

namespace mir
{
namespace logging
{
class Logger;
}
namespace graphics
{
/**
 * Install a EGL_KHR_debug callback that prints messages using \p logger
 *
 * This will override any previous EGL_KHR_debug callback set; it does not attempt to stack callbacks.
 *
 * \param logger [in]   The logger to print to.
 */
void initialise_egl_logger(std::shared_ptr<mir::logging::Logger> logger);
}
}

#endif //MIR_PLATFORM_GRAPHICS_EGL_LOGGER_H_
