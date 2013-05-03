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

#include <cstdarg>
#include <cstdio>

// TODO replace logging with mir reporting subsystem
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    if (prio < ANDROID_LOG_INFO) return 0;
    va_list ap;
    va_start(ap, fmt);
    int result = vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    return result;
}

extern "C" void __android_log_assert(const char *cond, const char *tag,
              const char *fmt, ...)
{
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    } else {
        /* Msg not provided, log condition.  N.B. Do not use cond directly as
         * format string as it could contain spurious '%' syntax (e.g.
         * "%d" in "blocks%devs == 0").
         */
        if (cond)
            fprintf(stderr, "Assertion failed: %s\n", cond);
        else
            fprintf(stderr, "Unspecified assertion failed\n");
    }

    __builtin_trap(); /* trap so we have a chance to debug the situation */
}
