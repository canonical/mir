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

#ifndef MIRAL_DECORATION_BASIC_MANAGER_H
#define MIRAL_DECORATION_BASIC_MANAGER_H

#include "miral/decoration.h"
#include <memory>

namespace mir
{
namespace shell
{
class Shell;
namespace decoration
{
class Decoration;
}
}
namespace scene
{
class Surface;
}
}

namespace miral
{
class DecorationManagerAdapter;
class DecorationBasicManager
{
public:
    DecorationBasicManager(Decoration::DecorationBuilder);

    auto to_adapter() -> std::shared_ptr<DecorationManagerAdapter>;
private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
