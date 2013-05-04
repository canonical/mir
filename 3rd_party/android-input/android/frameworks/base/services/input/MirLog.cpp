/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <std/Log.h>
#include <std/MirLog.h>
#include <cstdarg>
#include <cstdio>


extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    char buffer[1024] = {'0'};
    va_list ap;
    va_start(ap, fmt);
    int result = vsnprintf(buffer, sizeof buffer - 1, fmt, ap);
    va_end(ap);

    mir::write_to_log(prio, buffer);

    return result;
}

extern "C" void __android_log_assert(const char *cond, const char *tag,
              const char *fmt, ...)
{
    char buffer[1024] = {'0'};
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buffer, sizeof buffer - 1, fmt, ap);
        va_end(ap);

        mir::write_to_log(ANDROID_LOG_ERROR, buffer);
    } else {
        /* Msg not provided, log condition.  N.B. Do not use cond directly as
         * format string as it could contain spurious '%' syntax (e.g.
         * "%d" in "blocks%devs == 0").
         */
        if (cond) {
            snprintf(buffer, sizeof buffer - 1, "Assertion failed: %s", cond);
            mir::write_to_log(ANDROID_LOG_ERROR, buffer);
        }
        else
            mir::write_to_log(ANDROID_LOG_ERROR, "Unspecified assertion failed");
    }

    __builtin_trap(); /* trap so we have a chance to debug the situation */
}

namespace
{
static void default_write_to_log(int /*prio*/, char const* /*buffer*/)
{
    // By default don't log
}
}

void (*mir::write_to_log)(int prio, char const* buffer) = ::default_write_to_log;
