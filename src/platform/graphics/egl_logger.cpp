/*
 * Copyright © Canonical Ltd.
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
 */


#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <mir/graphics/egl_logger.h>

#define MIR_LOG_COMPONENT "EGL"
#include <mir/log.h>
#include <mir/graphics/egl_extensions.h>
#include <mir/graphics/egl_error.h>

namespace mg = mir::graphics;

namespace
{
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
                mir::log_error("Unexpected EGL log level encountered: {}. This is a Mir programming error.", egl_severity);
                // Shrug. Let's pick error?
                return mir::logging::Severity::error;
            }
        }(message_type);

    mir::log(
        severity,
        MIR_LOG_COMPONENT,
        "[%s] on [%s]: %s (%s): %s",
        thread_id,
        object_id,
        command,
        mg::egl_category().message(error).c_str(),
        message);
}

}

void mg::initialise_egl_logger()
{
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
        mir::log_info("EGL_KHR_debug logging enabled at maximum verbosity");
    }
    else
    {
        mir::log_warning("Debug logging requested, but no EGL_KHR_debug support detected");
    }
}
