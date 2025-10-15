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

#include <iostream>

namespace mir
{

std::ostream& operator<<(std::ostream& out, MirInputEventModifier modifier);
std::ostream& operator<<(std::ostream& out, MirKeyboardAction action);
std::ostream& operator<<(std::ostream& out, MirTouchAction action);
std::ostream& operator<<(std::ostream& out, MirTouchTooltype tool);
std::ostream& operator<<(std::ostream& out, MirPointerAction action);
std::ostream& operator<<(std::ostream& out, MirPromptSessionState state);
std::ostream& operator<<(std::ostream& out, MirOrientation orientation);

std::ostream& operator<<(std::ostream& out, MirWindowAttrib attribute);
std::ostream& operator<<(std::ostream& out, MirWindowFocusState state);
std::ostream& operator<<(std::ostream& out, MirWindowVisibility state);
std::ostream& operator<<(std::ostream& out, MirWindowType type);
std::ostream& operator<<(std::ostream& out, MirWindowState state);

std::ostream& operator<<(std::ostream& out, MirPromptSessionEvent const& event);
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

#endif
