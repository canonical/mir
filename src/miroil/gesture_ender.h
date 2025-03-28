/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIROIL_GESTURE_ENDER_H
#define MIROIL_GESTURE_ENDER_H

#include <memory>

namespace mir { class Server; }
namespace mir::frontend { class PointerInputDispatcher; }

namespace miroil
{
auto the_pointer_input_dispatcher(mir::Server& server) -> std::shared_ptr<mir::frontend::PointerInputDispatcher>;
}

#endif // MIROIL_GESTURE_ENDER_H
