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

#ifndef MIR_SHELL_HOVER_CLICK_TRANSFORMER_H_
#define MIR_SHELL_HOVER_CLICK_TRANSFORMER_H_

#include <mir/input/transformer.h>

#include <chrono>

namespace mir
{
namespace shell
{
class HoverClickTransformer : public input::Transformer
{
public:
    virtual void hover_duration(std::chrono::milliseconds delay) = 0;
    virtual void cancel_displacement_threshold(int displacement) = 0;
    virtual void reclick_displacement_threshold(int displacement) = 0;

    virtual void on_hover_start(std::function<void()>&& on_hover_start) = 0;
    virtual void on_hover_cancel(std::function<void()>&& on_hover_cancelled) = 0;
    virtual void on_click_dispatched(std::function<void()>&& on_click_dispatched) = 0;
};
}
}

#endif // MIR_SHELL_HOVER_CLICK_TRANSFORMER_H_
