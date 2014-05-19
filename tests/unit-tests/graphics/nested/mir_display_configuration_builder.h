/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_MIR_DISPLAY_CONFIGURATION_BUILDER_H_
#define MIR_TEST_MIR_DISPLAY_CONFIGURATION_BUILDER_H_

#include "mir_toolkit/client_types.h"
#include <memory>

namespace mir
{
namespace test
{

std::shared_ptr<MirDisplayConfiguration> build_trivial_configuration();
std::shared_ptr<MirDisplayConfiguration> build_non_trivial_configuration();

}
}

#endif /* MIR_TEST_MIR_DISPLAY_CONFIGURATION_BUILDER_H_ */
