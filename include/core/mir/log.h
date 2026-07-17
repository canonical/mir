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

#include <mir/logging/logger.h> // for Severity

#include <format>
#include <source_location>
#include <string>
#include <cstdarg>
#include <exception>
#include <string_view>

namespace mir
{
[[gnu::format(printf, 3, 0)]]
void logv(
    logging::Severity sev,
    char const* component,
    char const* fmt,
    va_list va,
    std::source_location loc = std::source_location::current());

/*
 * The following is a bit weird because we want to have both a variadic
 * logging API (both the older printf-style and the new std::format-style),
 * but we also want to capture a std::source_location for each of the log sites
 * and the idiomatic way to do *that* is with a default
 * `std::source_location loc = std::source_location::current()` argument.
 * (This use of ::current() is defined in the C++ spec as resolving to the call site).
 *
 * That combination plays badly: you can't have both variadics and default arguments,
 * because how can the compiler tell what argument should be variadic and what should be
 * defaulted?
 *
 * We *can* resolve this for the compiler by using a user-provided template deduction
 * guide.
 *
 * Unfortunately, template deduction guides only apply to templated *types*, not
 * templated *functions*.
 *
 * So: mir::log becomes a templated *type* with a family of partial- and
 * complete-specialisations, and mir::log(...) becomes calls to the constructors
 * of those types.
 *
 * Each of the old overloads of mir::log() becomes a (partial-)specialisation,
 * with a deduction guide to direct the compiler to the correct specialisation.
 *
 * The non-variadic APIs get two deduction guides - one with an explicit
 * std::source_location parameter and one without. That lets callers either omit
 * the std::source_location if they *are* the site that should be recorded as the
 * source of the log, or accept a std::source_location themselves and pass that on
 * if they're proxying for a different call-site.
 *
 * The variadic APIs can only have the single deduction guide; if a caller wants
 * to pass an explicit source_location they will need to manually select the
 * correct specialisation. For example:
 * ```
 *    std::source_location loc;
 *    mir::log<logging::Severity, char const*, char const*, Args...>::log(..., loc);
 * ```
 */
template<typename ...Args>
struct log;

template<typename ...Args>
struct log<logging::Severity, char const*, char const*, Args...> final
{
    log(
        logging::Severity sev,
        char const* component,
        char const* fmt,
        Args... args,
        std::source_location loc = std::source_location::current())
    {
        auto const forwarder =
            [sev, component, loc](char const* fmt, ...)
            {
                va_list va;
                va_start(va, fmt);
                mir::logv(sev, component, fmt, va, loc);
                va_end(va);
            };
        forwarder(fmt, args...);
    }
};
template<typename ...Args>
log(logging::Severity, char const*, char const*, Args...)
    -> log<logging::Severity, char const*, char const*, Args...>;

template<>
struct log<logging::Severity, char const*, std::string const&> final
{
    log(
        logging::Severity sev,
        char const* component,
        std::string const& message,
        std::source_location loc = std::source_location::current());
};
log(logging::Severity, char const*, std::string const&)
    -> log<logging::Severity, char const*, std::string const&>;
log(logging::Severity, char const*, std::string const&, std::source_location)
    -> log<logging::Severity, char const*, std::string const&>;

template<>
struct log<logging::Severity, char const*, std::exception_ptr const&, std::string const&> final
{
    log(
        logging::Severity sev,
        char const* component,
        std::exception_ptr const& exception,
        std::string const& message,
        std::source_location log = std::source_location::current());
};
log(logging::Severity, char const*, std::exception_ptr const&, std::string const&)
    -> log<logging::Severity, char const*, std::exception_ptr const&, std::string const&>;
log(logging::Severity, char const*, std::exception_ptr const&, std::string const&, std::source_location)
    -> log<logging::Severity, char const*, std::exception_ptr const&, std::string const&>;


template<typename... Args>
struct log<logging::Severity, logging::Tags, std::format_string<Args...>, Args...> final
{
    log(
        logging::Severity severity,
        logging::Tags tags,
        std::format_string<Args...> fmt,
        Args&&... args,
        std::source_location loc = std::source_location::current())
    {
        log<logging::Severity, logging::Tags, std::string_view>(severity, tags, std::format(fmt, std::forward<Args>(args)...), loc);
    }
};
template<typename... Args>
log(logging::Severity, logging::Tags, std::format_string<Args...>, Args&&...)
    -> log<logging::Severity, logging::Tags, std::format_string<Args...>, Args...>;

template<>
struct log<logging::Severity, logging::Tags, std::string_view> final
{
    log(logging::Severity sev,
        logging::Tags tags,
        std::string_view message,
        std::source_location loc = std::source_location::current());
};
log(logging::Severity, logging::Tags, std::string_view) -> log<logging::Severity, logging::Tags, std::string_view>;
log(logging::Severity, logging::Tags, std::string_view, std::source_location)
        -> log<logging::Severity, logging::Tags, std::string_view>;

/// Log a security event according to the OWASP specification
///
/// \param severity The severity of the event
/// \param event A short string identifying the event type
/// \param description A human-readable description of the event
void security_log(logging::Severity severity, std::string const& event, std::string const& description);

#ifndef MIR_LOG_COMPONENT
  #ifdef MIR_LOG_COMPONENT_FALLBACK
    #define MIR_LOG_COMPONENT MIR_LOG_COMPONENT_FALLBACK
  #endif
#endif

namespace
{
// Isolated namespace so that the component string is always correct for
// where it's used.
//
// For C++-overload-resolution reasons these always-available functions need to be
// defined in the same namespace as the conditionally-available ones below.

/*
 * As above, the log_{debug,info,warn,error,critical} family need to be
 * types-with-deduction-guides
 */
template<typename... Args>
struct log_debug;

template<>
struct log_debug<logging::Tags, std::string_view>
{
    log_debug(
        logging::Tags tags,
        std::string_view message,
        std::source_location loc = std::source_location::current())
    {
        mir::log(logging::Severity::debug, tags, message, loc);
    }
};
log_debug(logging::Tags, std::string_view) -> log_debug<logging::Tags, std::string_view>;
log_debug(logging::Tags, std::string_view, std::source_location)
    -> log_debug<logging::Tags, std::string_view>;

template<typename... Args>
struct log_debug<logging::Tags, std::format_string<Args...>, Args...>
{
    log_debug(
        logging::Tags tags,
        std::format_string<Args...> fmt,
        Args&&... args,
        std::source_location const& location = std::source_location::current())
    {
        mir::log<logging::Severity, logging::Tags, std::format_string<Args...>, Args...>(
            logging::Severity::debug,
            tags,
            fmt,
            std::forward<Args>(args)...,
            location);
    }
};
template<typename ...Args>
log_debug(logging::Tags, std::format_string<Args...>, Args&&...)
    -> log_debug<logging::Tags, std::format_string<Args...>, Args...>;

inline void log_info(logging::Tags tags, std::string_view message)
{ mir::log(logging::Severity::informational, tags, message); }

template<typename... Args>
void log_info(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{ log_info(tags, std::format(fmt, std::forward<Args>(args)...)); }

inline void log_warning(logging::Tags tags, std::string_view message)
{ mir::log(logging::Severity::warning, tags, message); }

template<typename... Args>
void log_warning(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{ log(logging::Severity::warning, tags, fmt, std::forward<Args>(args)...); }

inline void log_error(logging::Tags tags, std::string_view message)
{ mir::log(logging::Severity::error, tags, message); }

template<typename... Args>
void log_error(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{ log(logging::Severity::error, tags, fmt, std::forward<Args>(args)...); }

inline void log_critical(logging::Tags tags, std::string_view message)
{ mir::log(logging::Severity::critical, tags, message); }

template<typename... Args>
void log_critical(logging::Tags tags, std::format_string<Args...> fmt, Args&&... args)
{ log(logging::Severity::critical, tags, fmt, std::forward<Args>(args)...); }

#ifdef MIR_LOG_COMPONENT

inline void log_info(std::string const& message)
{ ::mir::log(::mir::logging::Severity::informational, MIR_LOG_COMPONENT, message); }

[[gnu::format(printf, 1, 2)]]
inline void log_info(char const* fmt, ...)
{
    va_list va;
    ::mir::logv(::mir::logging::Severity::informational, MIR_LOG_COMPONENT, fmt, va);
    va_start(va, fmt);
    va_end(va);
}

[[gnu::format(printf, 1, 2)]]
inline void log_error(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::error, MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}

template<typename... Args>
struct log_debug<char const*, Args...>
{
    log_debug(
        char const* fmt,
        Args... args,
        std::source_location const& location = std::source_location::current())
    {
        mir::log<logging::Severity, char const*, char const*, Args...>(
            logging::Severity::debug,
            MIR_LOG_COMPONENT,
            fmt,
            args...,
            location);
    }
};
template<typename ...Args>
log_debug(char const*, Args...) -> log_debug<char const*, Args...>;

inline void log_critical(std::string const& message)
{ ::mir::log(::mir::logging::Severity::critical, MIR_LOG_COMPONENT, message); }

[[gnu::format(printf, 1, 2)]]
inline void log_critical(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::critical, MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}

inline void log_error(std::string const& message)
{ ::mir::log(::mir::logging::Severity::error, MIR_LOG_COMPONENT, message); }

inline void log_warning(std::string const& message)
{ ::mir::log(::mir::logging::Severity::warning, MIR_LOG_COMPONENT, message); }

[[gnu::format(printf, 1, 2)]]
inline void log_warning(char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ::mir::logv(::mir::logging::Severity::warning, MIR_LOG_COMPONENT, fmt, va);
    va_end(va);
}
#endif
} // (nested anonymous) namespace

} // namespace mir

#endif // MIR_LOG_H_
