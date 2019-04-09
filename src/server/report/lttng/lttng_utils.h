/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include <lttng/tracepoint.h>
#include <stdint.h>

#ifndef _MIR_LTTNG_UTILS_H_
#define _MIR_LTTNG_UTILS_H_

#define MIR_LTTNG_VOID_TRACE_CALL(klass, comp, name) \
    void mir::report::lttng::klass::name()           \
    {                                                \
        mir_tracepoint(comp, name);               \
    }

#define MIR_LTTNG_VOID_TRACE_CLASS(comp) \
    TRACEPOINT_EVENT_CLASS(comp, dummy_event, TP_ARGS(), TP_FIELDS())
#define MIR_LTTNG_VOID_TRACE_POINT(comp, name) \
    TRACEPOINT_EVENT_INSTANCE(comp, dummy_event, name, TP_ARGS(void))

#endif
