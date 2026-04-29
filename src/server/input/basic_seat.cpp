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

#include "basic_seat.h"
#include <mir/input/device.h>
#include <mir/input/input_sink.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/graphics/display_configuration.h>
#include <mir/geometry/rectangle.h>
#include <mir/shell/display_configuration_controller.h>
#include <mir_toolkit/common.h>

#include <algorithm>
#include <array>
#include <map>

#define MIR_LOG_COMPONENT "basicseat"
#include <mir/log.h>

namespace mi = mir::input;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

struct mi::BasicSeat::OutputTracker : mg::DisplayConfigurationObserver
{
    OutputTracker(SeatInputDeviceTracker& tracker,
                  std::function<std::shared_ptr<shell::DisplayConfigurationController>()> display_configuration_controller) :
         input_state_tracker{tracker},
         display_configuration_controller{std::move(display_configuration_controller)}
    {
    }

    void update_outputs(mg::DisplayConfiguration const& conf)
    {
        std::lock_guard lock(output_mutex);
        outputs.clear();
        geom::Rectangles output_rectangles;
        conf.for_each_output(
            [this, &output_rectangles](mg::DisplayConfigurationOutput const& output)
            {
                if (!output.used || !output.connected)
                    return;
                if (!output.valid() || (output.current_mode_index >= output.modes.size()))
                    return;

                // TODO make the decision whether display that is used but powered off should emit
                // touch screen events in a policy
                bool active = output.power_mode == mir_power_mode_on;

                auto output_size = output.modes[output.current_mode_index].size;
                auto width = output_size.width.as_int();
                auto height = output_size.height.as_int();
                OutputInfo::Matrix output_matrix{{
                    1.0f, 0.0f, float(output.top_left.x.as_int()),
                    0.0f, 1.0f, float(output.top_left.y.as_int())}};

                switch (output.orientation)
                {
                case mir_orientation_left:
                    output_matrix[0] = 0;
                    output_matrix[1] = -1;
                    output_matrix[2] += height;
                    output_matrix[3] = 1;
                    output_matrix[4] = 0;
                    break;
                case mir_orientation_right:
                    output_matrix[0] = 0;
                    output_matrix[1] = 1;
                    output_matrix[3] = -1;
                    output_matrix[4] = 0;
                    output_matrix[5] += width;
                    break;
                case mir_orientation_inverted:
                    output_matrix[0] = -1;
                    output_matrix[2] += width;
                    output_matrix[4] = -1;
                    output_matrix[5] += height;
                    break;
                default:
                    break;
                }
                if (active)
                    output_rectangles.add(output.extents());
                outputs.insert(std::make_pair(output.id.as_value(), OutputInfo{active, output_size, output_matrix}));
            });
        input_state_tracker.update_outputs(output_rectangles);
        bounding_rectangle = output_rectangles.bounding_rectangle();
    }

    void set_tablet_mode(bool enabled)
    {
        auto const controller = display_configuration_controller();
        if (!controller)
        {
            mir::log_warning("Tablet mode change ignored: display configuration controller unavailable");
            return;
        }

        mir::log_warning("Tablet mode %sabled, updating display configuration", enabled ? "en" : "dis");
        auto config = controller->base_configuration();
        config->for_each_output(
            [enabled](mg::UserDisplayConfigurationOutput& output)
            {
                // Find an internal display first
                if (output.type == mg::DisplayConfigurationOutputType::lvds
                    || output.type == mg::DisplayConfigurationOutputType::edp)
                {
                    output.form_factor = enabled ? mir_form_factor_tablet : mir_form_factor_monitor;
                }
            }
        );

        controller->set_base_configuration(config);
    }

    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        update_outputs(*config.get());
    }

    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        update_outputs(*config.get());
    }

    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {}

    void session_configuration_applied(std::shared_ptr<ms::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration> const&) override
    {}

    void session_configuration_removed(std::shared_ptr<ms::Session> const&) override
    {}

    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {}

    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {}

    void configuration_updated_for_session(
        std::shared_ptr<ms::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {}

    geom::Rectangle get_bounding_rectangle() const
    {
        std::lock_guard lock(output_mutex);
        return bounding_rectangle;
    }

    mi::OutputInfo get_output_info(uint32_t output) const
    {
        std::lock_guard lock(output_mutex);
        if (output)
        {
            auto pos = outputs.find(output);
            if (pos != end(outputs))
                return pos->second;
        }
        else
        {
            // Output has not been populated sensibly, that's expected as there's no way to do that (yet).
            // FIXME: We just guess (which works with a single touchscreen output). {alan_g}
            // https://github.com/canonical/mir/issues/611
            auto const pos = begin(outputs);
            if (pos != end(outputs))
                return pos->second;
        }
        return OutputInfo{};
    }

private:
    mutable std::mutex output_mutex;
    mi::SeatInputDeviceTracker& input_state_tracker;
    std::map<uint32_t, mi::OutputInfo> outputs;
    geom::Rectangle bounding_rectangle;
    std::function<std::shared_ptr<shell::DisplayConfigurationController>()> display_configuration_controller;
};

mi::BasicSeat::BasicSeat(std::shared_ptr<mi::InputDispatcher> const& dispatcher,
                         std::shared_ptr<mi::TouchVisualizer> const& touch_visualizer,
                         std::shared_ptr<mi::CursorObserver> const& cursor_observer,
                         std::shared_ptr<Registrar> const& registrar,
                         std::shared_ptr<mi::KeyMapper> const& key_mapper,
                         std::shared_ptr<time::Clock> const& clock,
                         std::shared_ptr<mi::SeatObserver> const& observer,
                         std::function<std::shared_ptr<mir::shell::DisplayConfigurationController>()> display_configuration_controller) :
      input_state_tracker{dispatcher,
                          touch_visualizer,
                          cursor_observer,
                          key_mapper,
                          clock,
                          observer},
      output_tracker{std::make_shared<OutputTracker>(input_state_tracker, std::move(display_configuration_controller))}
{
    registrar->register_interest(output_tracker);
}

void mi::BasicSeat::add_device(input::Device const& device)
{
    input_state_tracker.add_device(device.id());
    if (contains(device.capabilities(), mi::DeviceCapability::pointer))
        input_state_tracker.add_pointing_device();
}

void mi::BasicSeat::remove_device(input::Device const& device)
{
    if (contains(device.capabilities(), mi::DeviceCapability::pointer))
        input_state_tracker.remove_pointing_device();
    input_state_tracker.remove_device(device.id());
}

void mi::BasicSeat::dispatch_event(std::shared_ptr<MirEvent> const& event)
{
    // Intercept the switch events here so we can appropriately update display configurations
    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        auto input_event = mir_event_get_input_event(event.get());
        if (mir_input_event_get_type(input_event) == mir_input_event_type_switch)
        {
            auto switch_event = mir_input_event_get_switch_event(input_event);
            if (mir_switch_event_action(switch_event) == mir_switch_action_tablet_mode)
                output_tracker->set_tablet_mode(mir_switch_event_state(switch_event) == mir_switch_state_on);

            // We don't want to dispatch the tablet mode switch event to the rest of the system, as it's not an input event that clients should be aware of.
            return;
        }
    }

    input_state_tracker.dispatch(event);
}

geom::Rectangle mi::BasicSeat::bounding_rectangle() const
{
    return output_tracker->get_bounding_rectangle();
}

mi::OutputInfo mi::BasicSeat::output_info(uint32_t output_id) const
{
    return output_tracker->get_output_info(output_id);
}

mir::EventUPtr mi::BasicSeat::create_device_state()
{
    return input_state_tracker.create_device_state();
}

auto mi::BasicSeat::xkb_modifiers() const -> MirXkbModifiers
{
    return input_state_tracker.xkb_modifiers();
}

void mi::BasicSeat::set_key_state(Device const& dev, std::vector<uint32_t> const& scan_codes)
{
    input_state_tracker.set_key_state(dev.id(), scan_codes);
}

void mi::BasicSeat::set_pointer_state(Device const& dev, MirPointerButtons buttons)
{
    input_state_tracker.set_pointer_state(dev.id(), buttons);
}

void mi::BasicSeat::set_cursor_position(float cursor_x, float cursor_y)
{
    input_state_tracker.set_cursor_position(cursor_x, cursor_y);
}

void mi::BasicSeat::set_confinement_regions(geom::Rectangles const& regions)
{
    input_state_tracker.set_confinement_regions(regions);
}

void mi::BasicSeat::reset_confinement_regions()
{
    input_state_tracker.reset_confinement_regions();
}
