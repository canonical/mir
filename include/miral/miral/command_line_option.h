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

#ifndef MIRAL_COMMAND_LINE_OPTION_H
#define MIRAL_COMMAND_LINE_OPTION_H

#include <miral/configuration_option.h>

namespace miral
{
/// A command line option that can be added to Mir's internal option
/// handling.
///
/// \deprecated Prefer using #miral::ConfigurationOption instead.
using CommandLineOption = ConfigurationOption;
}

#endif //MIRAL_COMMAND_LINE_OPTION_H
