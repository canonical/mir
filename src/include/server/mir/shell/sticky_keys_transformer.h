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

#ifndef MIR_SHELL_STICKY_KEYS_TRANSFORMER_H
#define MIR_SHELL_STICKY_KEYS_TRANSFORMER_H

#include "mir/input/input_event_transformer.h"
#include <functional>

namespace mir::shell
{

class StickyKeysTransformer : public input::InputEventTransformer::Transformer
{
public:
    virtual void should_disable_if_two_keys_are_pressed_together(bool) = 0;
    virtual void on_modifier_clicked(std::function<void(int32_t)>&&) = 0;
};

}

#endif //MIR_SHELL_STICKY_KEYS_TRANSFORMER_H
