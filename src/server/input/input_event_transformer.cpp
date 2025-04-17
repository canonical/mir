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

#include "mir/log.h"

#include <algorithm>

namespace mi = mir::input;

mi::InputEventTransformer::InputEventTransformer()
{
}

mir::input::InputEventTransformer::~InputEventTransformer() = default;

bool mi::InputEventTransformer::transform(MirEvent const& event, EventBuilder* builder, EventDispatcher const& dispatcher)
{
    std::lock_guard lock{mutex};

    for (auto it = input_transformers.begin(); it != input_transformers.end();)
    {
        if (it->expired())
        {
            it = input_transformers.erase(it);
        }
        else
        {
            ++it;
        }
    }

    input_transformers.shrink_to_fit();

    auto handled = false;

    for (auto it : input_transformers)
    {
        auto const& t = it.lock();

        if (t->transform_input_event(dispatcher, builder, event))
        {
            handled = true;
            break;
        }
    }

    return handled;
}

void mi::InputEventTransformer::append(std::weak_ptr<mi::InputEventTransformer::Transformer> const& transformer)
{
    std::lock_guard lock{mutex};

    auto const duplicate_iter = std::ranges::find(
            input_transformers,
            transformer.lock().get(),
            [](auto const& other_transformer)
            {
                return other_transformer.lock().get();
            });

    if (duplicate_iter != input_transformers.end())
        return;

    input_transformers.push_back(transformer);
}

bool mi::InputEventTransformer::remove(std::shared_ptr<mi::InputEventTransformer::Transformer> const& transformer)
{
    std::lock_guard lock{mutex};
    auto [remove_start, remove_end] = std::ranges::remove(
        input_transformers,
        transformer,
        [](auto const& list_element)
        {
            return list_element.lock();
        });

    if (remove_start == input_transformers.end())
    {
        mir::log_error("Attempted to remove a transformer that doesn't exist in `input_transformers`");
        return false;
    }

    input_transformers.erase(remove_start, remove_end);
    return true;
}
