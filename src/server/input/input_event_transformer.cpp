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
#include "default_event_builder.h"
#include "mir/events/event_builders.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/seat.h"
#include "mir/input/virtual_input_device.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_touchscreen_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/main_loop.h"

#include <algorithm>

namespace mi = mir::input;

mi::InputEventTransformer::InputEventTransformer(
    std::shared_ptr<Seat> const& seat, std::shared_ptr<time::Clock> const& clock) :
    seat{seat},
    clock{clock},
    builder{std::make_unique<DefaultEventBuilder>(id, clock)}
{
    struct StubDevice : public Device
    {
        StubDevice(MirInputDeviceId id, DeviceCapabilities capabilities, std::string name) :
            id_{id},
            capabilities_{capabilities},
            name_{name},
            unique_id_{name}
        {
        }

        MirInputDeviceId id() const override
        {
            return id_;
        }
        DeviceCapabilities capabilities() const override
        {
            return capabilities_;
        }
        std::string name() const override
        {
            return name_;
        }
        std::string unique_id() const override
        {
            return unique_id_;
        }
        mir::optional_value<MirPointerConfig> pointer_configuration() const override
        {
            return mir::optional_value<MirPointerConfig>{};
        }
        void apply_pointer_configuration(MirPointerConfig const&) override
        {
        }
        mir::optional_value<MirTouchpadConfig> touchpad_configuration() const override
        {
            return mir::optional_value<MirTouchpadConfig>{};
        }
        void apply_touchpad_configuration(MirTouchpadConfig const&) override
        {
        }
        optional_value<MirKeyboardConfig> keyboard_configuration() const override
        {
            return mir::optional_value<MirKeyboardConfig>{};
        }
        void apply_keyboard_configuration(MirKeyboardConfig const&) override
        {
        }
        optional_value<MirTouchscreenConfig> touchscreen_configuration() const override
        {
            return mir::optional_value<MirTouchscreenConfig>{};
        }
        void apply_touchscreen_configuration(MirTouchscreenConfig const&) override
        {
        }

    private:
        MirInputDeviceId id_;
        DeviceCapabilities capabilities_;
        std::string name_;
        std::string unique_id_;
    };
    auto const handle = StubDevice(
        id, DeviceCapabilities{DeviceCapability::pointer} | DeviceCapability::keyboard, "accessibility-device");

    seat->add_device(handle);
}

mir::input::InputEventTransformer::~InputEventTransformer() = default;

bool mi::InputEventTransformer::transform(
    MirEvent const& event, EventBuilder* builder, EventDispatcher const& dispatcher)
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

bool mi::InputEventTransformer::append(std::weak_ptr<mi::InputEventTransformer::Transformer> const& transformer)
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
        return false;

    input_transformers.push_back(transformer);
    return true;
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

void mir::input::InputEventTransformer::add_device(Device const& device)
{
    seat->add_device(device);
}

void mir::input::InputEventTransformer::remove_device(Device const& device)
{
    seat->remove_device(device);
}

void mir::input::InputEventTransformer::dispatch_event(std::shared_ptr<MirEvent> const& event)
{
    if(!transform(*event, builder.get(), [this](auto event) { seat->dispatch_event(event); }))
        seat->dispatch_event(event); 
}

mir::EventUPtr mir::input::InputEventTransformer::create_device_state()
{
    return seat->create_device_state();
}

auto mir::input::InputEventTransformer::xkb_modifiers() const -> MirXkbModifiers
{
    return seat->xkb_modifiers();
}

void mir::input::InputEventTransformer::set_key_state(Device const& dev, std::vector<uint32_t> const& scan_codes)
{
    seat->set_key_state(dev, scan_codes);
}

void mir::input::InputEventTransformer::set_pointer_state(Device const& dev, MirPointerButtons buttons)
{
    seat->set_pointer_state(dev, buttons);
}

void mir::input::InputEventTransformer::set_cursor_position(float cursor_x, float cursor_y)
{
    seat->set_cursor_position(cursor_x, cursor_y);
}

void mir::input::InputEventTransformer::set_confinement_regions(geometry::Rectangles const& regions)
{
    seat->set_confinement_regions(regions);
}

void mir::input::InputEventTransformer::reset_confinement_regions()
{
    seat->reset_confinement_regions();
}

mir::geometry::Rectangle mir::input::InputEventTransformer::bounding_rectangle() const
{
    return seat->bounding_rectangle();
}

mir::input::OutputInfo mir::input::InputEventTransformer::output_info(uint32_t output_id) const
{
    return seat->output_info(output_id);
}
