/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 * Fatal error handling - Fatal errors are situations we don't expect to ever
 * happen and don't have logic to gracefully recover from. The most useful
 * thing you can do in that situation is abort to get a clean core file and
 * stack trace to maximize the chances of it being readable.
 */

#ifndef MIR_FATAL_H_
#define MIR_FATAL_H_

namespace mir
{
/**
 * fatal_error() is strictly for "this should never happen" situations that
 * you cannot recover from. By default it points at fatal_error_abort().
 * Note the reason parameter is a simple char* so its value is clearly visible
 * in stack trace output.
 * \remark There is no attempt to make this thread-safe, if it needs to be changed
 * \remark Since MirCore 0.25
 * that should be done before spinning up the Mir server.
 *   \param [in] reason  A printf-style format string.
 */
extern void (*fatal_error)(char const* reason, ...);

/**
 * Throws an exception that will typically kill the Mir server and propagate from
 * mir::run_mir.
 * \remark Since MirCore 0.25
 *   \param [in] reason  A printf-style format string.
 */
void fatal_error_except(char const* reason, ...);

/**
 * An alternative to fatal_error_except() that kills the program and dump core
 * as cleanly as possible.
 * \remark Since MirCore 0.25
 *   \param [in] reason  A printf-style format string.
 */
[[noreturn]]
void fatal_error_abort(char const* reason, ...);

// Utility class to override & restore existing error handler
class FatalErrorStrategy
{
public:
    explicit FatalErrorStrategy(void (*fatal_error_handler)(char const* reason, ...)) :
        old_fatal_error_handler(fatal_error)
    {
        fatal_error = fatal_error_handler;
    }

    ~FatalErrorStrategy()
    {
        fatal_error = old_fatal_error_handler;
    }

private:
    void (*old_fatal_error_handler)(char const* reason, ...);
    FatalErrorStrategy(FatalErrorStrategy const&) = delete;
    FatalErrorStrategy& operator=(FatalErrorStrategy const&) = delete;
};
} // namespace mir

#endif // MIR_FATAL_H_
