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
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

// License of the original configureInitialState() function:
/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include <std/MirLog.h>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <atomic>

#define kMaxTagLen  16      /* from the long-dead utils/Log.cpp */
#define kTagSetSize 16      /* arbitrary */

namespace
{
    struct LogState {
        /* global minimum priority */
        std::atomic<int> globalMinPriority{ANDROID_LOG_UNKNOWN};

        /* tags and priorities */
        struct {
            char    tag[kMaxTagLen];
            std::atomic<int> minPriority;
        } tagSet[kTagSetSize];
    } gLogState = {};
}

/*
 * Configure logging based on ANDROID_LOG_TAGS environment variable.  We
 * need to parse a string that looks like
 *
 *   *:v jdwp:d dalvikvm:d dalvikvm-gc:i dalvikvmi:i
 *
 * The tag (or '*' for the global level) comes first, followed by a colon
 * and a letter indicating the minimum priority level we're expected to log.
 * This can be used to reveal or conceal logs with specific tags.
 *
 * We also want to check ANDROID_PRINTF_LOG to determine how the output
 * will look.
 */
static void configureInitialState(struct LogState* logState)
{
    /* global min priority defaults to "info" level */
    logState->globalMinPriority = ANDROID_LOG_INFO;

    int entry = 0;

    /*
     * This is based on the the long-dead utils/Log.cpp code.
     */
    const char* tags = getenv("ANDROID_LOG_TAGS");
    if (tags != NULL) {
        while (*tags != '\0') {
            char tagName[kMaxTagLen];
            int i, minPrio;

            while (isspace(*tags))
                tags++;

            i = 0;
            while (*tags != '\0' && !isspace(*tags) && *tags != ':' &&
                i < kMaxTagLen)
            {
                tagName[i++] = *tags++;
            }
            if (i == kMaxTagLen) {
                break;
            }
            tagName[i] = '\0';

            /* default priority, if there's no ":" part; also zero out '*' */
            minPrio = ANDROID_LOG_VERBOSE;
            if (tagName[0] == '*' && tagName[1] == '\0') {
                minPrio = ANDROID_LOG_DEBUG;
                tagName[0] = '\0';
            }

            if (*tags == ':') {
                tags++;
                if (*tags >= '0' && *tags <= '9') {
                    if (*tags >= ('0' + ANDROID_LOG_SILENT))
                        minPrio = ANDROID_LOG_VERBOSE;
                    else
                        minPrio = *tags - '\0';
                } else {
                    switch (*tags) {
                    case 'v':   minPrio = ANDROID_LOG_VERBOSE;  break;
                    case 'd':   minPrio = ANDROID_LOG_DEBUG;    break;
                    case 'i':   minPrio = ANDROID_LOG_INFO;     break;
                    case 'w':   minPrio = ANDROID_LOG_WARN;     break;
                    case 'e':   minPrio = ANDROID_LOG_ERROR;    break;
                    case 'f':   minPrio = ANDROID_LOG_FATAL;    break;
                    case 's':   minPrio = ANDROID_LOG_SILENT;   break;
                    default:    minPrio = ANDROID_LOG_DEFAULT;  break;
                    }
                }

                tags++;
                if (*tags != '\0' && !isspace(*tags)) {
                    break;
                }
            }

            if (tagName[0] == 0) {
                logState->globalMinPriority = minPrio;
            } else {
                logState->tagSet[entry].minPriority = minPrio;
                strcpy(logState->tagSet[entry].tag, tagName);
                entry++;
            }
        }
    }

    if (entry < kTagSetSize) {
        // Mark the end of this array
        logState->tagSet[entry].minPriority = ANDROID_LOG_UNKNOWN;
        logState->tagSet[entry].tag[0] = '\0';
    }
}

extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    int result = 0;

    if (gLogState.globalMinPriority == ANDROID_LOG_UNKNOWN) {
        configureInitialState(&gLogState);
    }

    /* see if this log tag is configured */
    int minPrio = gLogState.globalMinPriority;
    for (int i = 0; i < kTagSetSize; i++) {
        if (gLogState.tagSet[i].minPriority == ANDROID_LOG_UNKNOWN)
            break;      /* reached end of configured values */

        if (strcmp(gLogState.tagSet[i].tag, tag) == 0) {
            minPrio = gLogState.tagSet[i].minPriority;
            break;
        }
    }

    if (prio >= minPrio) {
        char buffer[1024];
        int max = sizeof buffer - 1;
        int tagend = snprintf(buffer, max, "[%s]", tag);

        va_list ap;
        va_start(ap, fmt);
        result = vsnprintf(buffer+tagend, max - tagend, fmt, ap);
        va_end(ap);

        mir::write_to_log(prio, buffer);
    } else {
        // filter out log message
    }

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
