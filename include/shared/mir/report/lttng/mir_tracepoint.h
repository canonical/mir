/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_LTTNG_MIR_TRACEPOINT_H_
#define MIR_LTTNG_MIR_TRACEPOINT_H_

/*
 * The STAP_PROBEV() macro from sdt.h (SystemTap) used by
 * the tracepoint() macro from lttng/tracepoint.h fails to
 * build with clang at the moment. Disable tracepoints
 * when building with clang until this is resolved.
 *
 * See: http://sourceware.org/bugzilla/show_bug.cgi?id=13974
 */
#ifdef __clang__
namespace mir_systemtap_bug_13974
{
inline void mir_tracepoint_consume_args(int, ...) {}
}
#define mir_tracepoint(c, e, ...) ::mir_systemtap_bug_13974::mir_tracepoint_consume_args(0, __VA_ARGS__)
#pragma message "Building with clang: Disabling LTTng tracepoints."
#else
#define mir_tracepoint(c, ...) tracepoint(c, __VA_ARGS__)
#endif

#endif /* MIR_LTTNG_MIR_TRACEPOINT_H_ */
