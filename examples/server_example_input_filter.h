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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_EXAMPLES_EXAMPLE_INPUT_FILTER_H_
#define MIR_EXAMPLES_EXAMPLE_INPUT_FILTER_H_

#include <memory>

namespace mir
{
class Server;

namespace input { class EventFilter; }

namespace examples
{
auto make_printing_input_filter_for(mir::Server& server)
-> std::shared_ptr<input::EventFilter>;

auto make_screen_rotation_filter_for(mir::Server& server)
-> std::shared_ptr<input::EventFilter>;
}
}

#endif /* MIR_EXAMPLES_EXAMPLE_INPUT_FILTER_H_ */
