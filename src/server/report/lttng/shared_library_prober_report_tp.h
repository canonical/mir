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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_server_shared_library_prober

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./shared_library_prober_report_tp.h"

#if !defined(MIR_LTTNG_SHARED_LIBRARY_PROBER_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_SHARED_LIBRARY_PROBER_REPORT_TP_H_

#include "lttng_utils.h"

TRACEPOINT_EVENT(
    mir_server_shared_library_prober,
    probing_path,
    TP_ARGS(const char*, path),
    TP_FIELDS(
        ctf_string(path, path)
    )
)

TRACEPOINT_EVENT(
    mir_server_shared_library_prober,
    probing_failed,
    TP_ARGS(const char*, path, const char*, message),
    TP_FIELDS(
        ctf_string(path, path)
        ctf_string(message, message)
    )
)

TRACEPOINT_EVENT(
    mir_server_shared_library_prober,
    loading_library,
    TP_ARGS(const char*, path),
    TP_FIELDS(
        ctf_string(path, path)
    )
)

TRACEPOINT_EVENT(
    mir_server_shared_library_prober,
    loading_failed,
    TP_ARGS(const char*, path, const char*, message),
    TP_FIELDS(
        ctf_string(path, path)
        ctf_string(message, message)
    )
)

#endif /* MIR_LTTNG_SHARED_LIBRARY_PROBER_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
