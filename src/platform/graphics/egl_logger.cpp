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


#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "mir/graphics/egl_logger.h"

#include "mir/log.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"

namespace mg = mir::graphics;

namespace
{
std::shared_ptr<mir::logging::Logger> egl_logger;

void egl_debug_logger(
    EGLenum error,
    char const* command,
    EGLint message_type,
    EGLLabelKHR thread_label,
    EGLLabelKHR object_label,
    char const* message)
{
    char const* const thread_id = thread_label ? static_cast<char const*>(thread_label) : "Unlabelled thread";
    char const* const object_id = object_label ? static_cast<char const*>(object_label) : "Unlabelled object";

    auto const severity =
        [](EGLint egl_severity)
        {
            switch (egl_severity)
            {
                case EGL_DEBUG_MSG_INFO_KHR:
                return mir::logging::Severity::debug;
                case EGL_DEBUG_MSG_WARN_KHR:
                return mir::logging::Severity::warning;
                case EGL_DEBUG_MSG_ERROR_KHR:
                return mir::logging::Severity::error;
                case EGL_DEBUG_MSG_CRITICAL_KHR:
                return mir::logging::Severity::critical;
                default:
                egl_logger->log(
                    "EGL",
                    mir::logging::Severity::error,
                    "Unexpected EGL log level encountered: %i. This is a Mir programming error.",
                    egl_severity);
                // Shrug. Let's pick error?
                return mir::logging::Severity::error;
            }
        }(message_type);

    egl_logger->log(
        "EGL",
        severity,
        "[%s] on [%s]: %s (%s): %s",
        thread_id,
        object_id,
        command,
        mg::egl_category().message(error).c_str(),
        message);
}

}

void mg::initialise_egl_logger(std::shared_ptr<mir::logging::Logger> logger)
{
    egl_logger = std::move(logger);
    if (auto debug_khr = mg::EGLExtensions::DebugKHR::maybe_debug_khr())
    {
        EGLAttrib enable_all_logging[] = {
            EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
            EGL_DEBUG_MSG_ERROR_KHR, EGL_TRUE,
            EGL_DEBUG_MSG_WARN_KHR, EGL_TRUE,
            EGL_DEBUG_MSG_INFO_KHR, EGL_TRUE,
            EGL_NONE
        };
        debug_khr->eglDebugMessageControlKHR(&egl_debug_logger, enable_all_logging);
        egl_logger->log(
            "EGL",
            mir::logging::Severity::informational,
            "EGL_KHR_debug logging enabled at maximum verbosity");
    }
    else
    {
        egl_logger->log(
            "EGL",
            mir::logging::Severity::informational,
            "No EGL_KHR_debug support detected");
    }
}
