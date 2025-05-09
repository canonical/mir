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

#include "mir/input/input_event_transformer.h"


namespace mir
{
class MainLoop;
namespace shell
{
class SlowKeysTransformer : public mir::input::InputEventTransformer::Transformer
{
public:
    virtual void on_key_down(std::function<void(unsigned int)>&&) = 0;
    virtual void on_key_rejected(std::function<void(unsigned int)>&&) = 0;
    virtual void on_key_accepted(std::function<void(unsigned int)>&&) = 0;

    virtual void delay(std::chrono::milliseconds) = 0;
};
}
}

