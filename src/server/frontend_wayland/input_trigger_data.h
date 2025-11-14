/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SERVER_FRONTEND_INPUT_TRIGGER_DATA_H_
#define MIR_SERVER_FRONTEND_INPUT_TRIGGER_DATA_H_

#include "mir/synchronised.h"
#include "mir/wayland/weak.h"
#include <unordered_map>

namespace mir
{
namespace frontend
{

class InputTriggerActionV1;

struct InputTriggerData
{
    mir::Synchronised<std::unordered_map<std::string, wayland::Weak<InputTriggerActionV1>>> registered_actions;
};

}
}

#endif
