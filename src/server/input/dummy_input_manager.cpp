/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/input/input_manager.h"

namespace mg = mir::graphics;
namespace mi = mir::input;

namespace
{
class DummyInputManager : public mi::InputManager
{
    void stop() {};
    void start() {};
}; 
}

std::shared_ptr<mi::InputManager> mi::create_input_manager(
    const std::initializer_list<std::shared_ptr<mi::EventFilter> const>& ,
    std::shared_ptr<mg::ViewableArea> const& )
{
    return std::make_shared<DummyInputManager>();
}
