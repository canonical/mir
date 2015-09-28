/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_POINTER_SETTINGS_H_
#define MIR_INPUT_POINTER_SETTINGS_H_

#include "mir_toolkit/client_types.h"

namespace mir
{
namespace input
{

struct PointerSettings
{
    /**
     * Configure left and right handed mode by selecting a primary button
     *   - mir_pointer_button_primary -> right handed
     *   - mir_pointer_button_secondary -> left handed
     */
    MirPointerButton primary_button{mir_pointer_button_primary};
    /**
     * Scale cursor accelaration.
     *   - 0: default acceleration
     *   - [-1, 0[: reduced acceleration
     *   - ]0, 1]: increased acceleration
     */
    double cursor_speed{0.0};
    double horizontal_scroll_speed{1.0};
    double vertical_scroll_speed{1.0};
};

}
}

#endif
