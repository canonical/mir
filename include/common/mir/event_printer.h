/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_EVENT_PRINTER_H_
#define MIR_EVENT_PRINTER_H_

#include <mir_toolkit/event.h>

#include <format>
#include <ostream>

namespace mir
{

std::ostream& operator<<(std::ostream& out, MirInputEventModifier modifier);
std::ostream& operator<<(std::ostream& out, MirKeyboardAction action);
std::ostream& operator<<(std::ostream& out, MirTouchAction action);
std::ostream& operator<<(std::ostream& out, MirTouchTooltype tool);
std::ostream& operator<<(std::ostream& out, MirPointerAction action);
std::ostream& operator<<(std::ostream& out, MirOrientation orientation);

std::ostream& operator<<(std::ostream& out, MirWindowAttrib attribute);
std::ostream& operator<<(std::ostream& out, MirWindowFocusState state);
std::ostream& operator<<(std::ostream& out, MirWindowVisibility state);
std::ostream& operator<<(std::ostream& out, MirWindowType type);
std::ostream& operator<<(std::ostream& out, MirWindowState state);

std::ostream& operator<<(std::ostream& out, MirResizeEvent const& event);
std::ostream& operator<<(std::ostream& out, MirOrientationEvent const& event);
std::ostream& operator<<(std::ostream& out, MirInputEvent const& event);
std::ostream& operator<<(std::ostream& out, MirCloseWindowEvent const& event);
std::ostream& operator<<(std::ostream& out, MirWindowEvent const& event);
std::ostream& operator<<(std::ostream& out, MirInputDeviceStateEvent const& event);
std::ostream& operator<<(std::ostream& out, MirWindowPlacementEvent const& event);
std::ostream& operator<<(std::ostream& out, MirWindowOutputEvent const& event);
std::ostream& operator<<(std::ostream& out, MirEvent const& event);

}

template<>
struct std::formatter<MirInputEventModifier> : std::formatter<std::string>
{
    auto format(MirInputEventModifier modifier, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirKeyboardAction> : std::formatter<std::string>
{
    auto format(MirKeyboardAction action, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirTouchAction> : std::formatter<std::string>
{
    auto format(MirTouchAction action, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirTouchTooltype> : std::formatter<std::string>
{
    auto format(MirTouchTooltype tool, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirPointerAction> : std::formatter<std::string>
{
    auto format(MirPointerAction action, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirOrientation> : std::formatter<std::string>
{
    auto format(MirOrientation orientation, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowAttrib> : std::formatter<std::string>
{
    auto format(MirWindowAttrib attribute, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowFocusState> : std::formatter<std::string>
{
    auto format(MirWindowFocusState state, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowVisibility> : std::formatter<std::string>
{
    auto format(MirWindowVisibility state, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowType> : std::formatter<std::string>
{
    auto format(MirWindowType type, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowState> : std::formatter<std::string>
{
    auto format(MirWindowState state, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirResizeEvent> : std::formatter<std::string>
{
    auto format(MirResizeEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirOrientationEvent> : std::formatter<std::string>
{
    auto format(MirOrientationEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirInputEvent> : std::formatter<std::string>
{
    auto format(MirInputEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirCloseWindowEvent> : std::formatter<std::string>
{
    auto format(MirCloseWindowEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowEvent> : std::formatter<std::string>
{
    auto format(MirWindowEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirInputDeviceStateEvent> : std::formatter<std::string>
{
    auto format(MirInputDeviceStateEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowPlacementEvent> : std::formatter<std::string>
{
    auto format(MirWindowPlacementEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirWindowOutputEvent> : std::formatter<std::string>
{
    auto format(MirWindowOutputEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

template<>
struct std::formatter<MirEvent> : std::formatter<std::string>
{
    auto format(MirEvent const& event, std::format_context& ctx) const -> std::format_context::iterator;
};

#endif
