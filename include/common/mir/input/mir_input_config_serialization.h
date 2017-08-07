/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_CONFIG_SERIALIZATION_H
#define MIR_INPUT_INPUT_CONFIG_SERIALIZATION_H

#include <string>

class MirInputConfig;
namespace mir
{
namespace input
{

std::string serialize_input_config(MirInputConfig const& config);
MirInputConfig deserialize_input_config(std::string const& buffer);

}
}

#endif
