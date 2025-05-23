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

#ifndef MIR_SHELL_BASIC_HOVER_CLICK_TRANSFORMER_H_
#define MIR_SHELL_BASIC_HOVER_CLICK_TRANSFORMER_H_

#include "mir/shell/hover_click_transformer.h"

#include "mir/geometry/point.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

namespace mir
{
class MainLoop;
namespace shell
{
class BasicHoverClickTransformer : public HoverClickTransformer
{
public:
    BasicHoverClickTransformer(std::shared_ptr<MainLoop> const& main_loop);

    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override;

    void hover_duration(std::chrono::milliseconds delay) override;
    void cancel_displacement_threshold(float displacement) override;

    void enabled() override;
    void disabled() override;
    void on_enabled(std::function<void()>&& on_enabled) override;
    void on_disabled(std::function<void()>&& on_disabled) override;

    void on_hover_start(std::function<void()>&& on_hover_start) override;
    void on_hover_cancel(std::function<void()>&& on_hover_cancelled) override;
    void on_click_dispatched(std::function<void()>&& on_click_dispatched) override;

private:
    void initialize_click_dispatcher(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher, mir::input::EventBuilder& builder);

    static constexpr auto grace_period_percentage = 0.1f;
    static constexpr auto hover_delay_percentage = 1.0f - grace_period_percentage;
    static constexpr auto reclick_displacement_threshold{10};

    struct MutableState 
    {
        std::unique_ptr<time::Alarm> click_dispatcher;

        // We repeatedly record the most recent cursor position. Once the grace
        // period is up, we "pin" this position into `hover_click_origin`, which is
        // used later on to cancel the hover click if the pointer moves too far
        // away.
        mir::geometry::PointF potential_position;
        std::optional<mir::geometry::PointF> hover_click_origin;

        std::chrono::milliseconds hover_duration{1000};
        float cancel_displacement_threshold{10};
        std::function<void()> on_enabled{[] {}};
        std::function<void()> on_disabled{[] {}};
        std::function<void()> on_hover_start{[] {}};
        std::function<void()> on_hover_cancel{[] {}};
        std::function<void()> on_click_dispatched{[] {}};
    };

    // Has to be initialized before the hover initializer since we access it
    // when initializing that.
    mir::Synchronised<MutableState> mutable_state;

    std::shared_ptr<MainLoop> const main_loop;

    // Provides a grace period, cursor motion events during this grace period
    // will not invoke the start/cancel callbacks.
    std::unique_ptr<time::Alarm> const hover_initializer;
};
}
}

#endif // MIR_SHELL_BASIC_HOVER_CLICK_TRANSFORMER_H_
