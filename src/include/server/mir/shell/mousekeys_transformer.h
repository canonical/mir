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

#ifndef MIR_SHELL_MOUSEKEYS_TRANSFORMER_H
#define MIR_SHELL_MOUSEKEYS_TRANSFORMER_H

#include "mir/input/transformer.h"
#include "mir/input/mousekeys_keymap.h"

#include <xkbcommon/xkbcommon-keysyms.h>

namespace mir
{
namespace shell
{
class MouseKeysTransformer: public mir::input::Transformer::Transformer
{
public:
    virtual void keymap(mir::input::MouseKeysKeymap const& new_keymap) = 0;
    virtual void acceleration_factors(double constant, double linear, double quadratic) = 0;
    virtual void max_speed(double x_axis, double y_axis) = 0;
};
}
}

#endif // MIR_SHELL_MOUSEKEYS_TRANSFORMER_H
