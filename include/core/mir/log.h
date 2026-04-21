/*
 * Convenience functions to make logging in Mir easy
 * ~~~
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

#ifndef MIR_LOG_H_
#define MIR_LOG_H_

#include <mir/logging/logger.h>  // for Severity

#include <format>
#include <string>
#include <cstdarg>
#include <exception>
#include <string_view>

namespace mir
{
[[gnu::format (printf, 3, 0)]]
void logv(logging::Severity sev, const char *component,
          char const* fmt, va_list va);
[[gnu::format (printf, 3, 4)]]
void log(logging::Severity sev, const char *component,
         char const* fmt, ...);
void log(logging::Severity sev, const char *component,
         std::string const& message);
void log(
    logging::Severity sev,
    char const* component,
    std::exception_ptr const& exception,
    std::string const& message);

template <typename... Args>
void log(logging::Severity severity, logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{
    log(severity, tags, std::format(fmt, std::forward<Args>(args)...));
}

void log(
    logging::Severity sev,
    logging::Tags tags,
    std::string_view message);

/// Log a security event according to the OWASP specification
///
/// \param severity The severity of the event
/// \param event A short string identifying the event type
/// \param description A human-readable description of the event
void security_log(
    logging::Severity severity,
    std::string const& event,
    std::string const& description);

#ifndef MIR_LOG_COMPONENT
#ifdef MIR_LOG_COMPONENT_FALLBACK
#define MIR_LOG_COMPONENT MIR_LOG_COMPONENT_FALLBACK
#endif
#endif

namespace {
// Isolated namespace so that the component string is always correct for
// where it's used.
//
// For C++-overload-resolution reasons these always-available functions need to be
// defined in the same namespace as the conditionally-available ones below.

template <typename... Args>
void log_debug(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{
    log(logging::Severity::debug, tags, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_info(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{
    log(logging::Severity::informational, tags, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_warning(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{
    log(logging::Severity::warning, tags, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_error(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{
    log(logging::Severity::error, tags, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_critical(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{
    log(logging::Severity::critical, tags, fmt, std::forward<Args>(args)...);
}

#ifdef MIR_LOG_COMPONENT

inline void log_info(std::string const& message)
{
    ::mir::log(::mir::logging::Severity::informational,
               MIR_LOG_COMPONENT, message);
}

[[gnu::format (printf, 1, 2)]]
inline void log_info(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::informational,
                MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}

[[gnu::format (printf, 1, 2)]]
inline void log_error(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::error,
                MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}

[[gnu::format (printf, 1, 2)]]
inline void log_debug(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::debug,
               MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}

inline void log_critical(std::string const& message)
{
    ::mir::log(::mir::logging::Severity::critical,
               MIR_LOG_COMPONENT, message);
}

[[gnu::format(printf, 1, 2)]]
inline void log_critical(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::critical,
               MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}

inline void log_error(std::string const& message)
{
    ::mir::log(::mir::logging::Severity::error,
               MIR_LOG_COMPONENT, message);
}

inline void log_warning(std::string const& message)
{
    ::mir::log(::mir::logging::Severity::warning,
               MIR_LOG_COMPONENT, message);
}

[[gnu::format(printf, 1, 2)]]
inline void log_warning(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::warning,
               MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}
#endif
} // (nested anonymous) namespace

} // namespace mir

#endif // MIR_LOG_H_
