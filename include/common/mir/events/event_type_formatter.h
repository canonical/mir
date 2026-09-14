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

#ifndef MIR_EVENT_TYPE_FORMATTER_H_
#define MIR_EVENT_TYPE_FORMATTER_H_

#include <mir_toolkit/events/enums.h>

#include <format>

template<>
struct std::formatter<MirEventType>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(MirEventType type, std::format_context& ctx) const -> std::format_context::iterator;
};

#endif // MIR_EVENT_TYPE_FORMATTER_H_
