/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_PLATFORM_FACTORY_H_
#define MIR_TEST_DOUBLES_PLATFORM_FACTORY_H_

#include <memory>

namespace mir
{
namespace graphics
{
class Platform;
namespace mesa { class Platform; }
}
namespace test
{
namespace doubles
{

std::shared_ptr<graphics::Platform> create_platform_with_null_dependencies();

#ifdef MESA_KMS
std::shared_ptr<graphics::mesa::Platform> create_mesa_platform_with_null_dependencies();
#endif

}
}
}

#endif /* MIR_TEST_DOUBLES_PLATFORM_FACTORY_H_ */
