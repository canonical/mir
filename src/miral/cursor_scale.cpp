/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/cursor_scale.h"
#include "mir/main_loop.h"
#include "mir/synchronised.h"
#include "miral/live_config.h"

#include "mir/options/option.h"
#include "mir/log.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/time/alarm.h"

#include <chrono>
#include <memory>

struct miral::CursorScale::Self
{
    Self(float default_scale) :
        default_scale{default_scale},
        state{State{default_scale}}
    {
    }

    void scale(float new_scale)
    {
        auto s = state.lock();

        if(new_scale == s->scale)
            return;

        s->scale = new_scale;

        if(accessibility_manager.expired())
            return;

        accessibility_manager.lock()->cursor_scale(s->scale);
    }

    void scale_temporarily(float scale_multiplier, std::chrono::milliseconds duration)
    {
        if(accessibility_manager.expired() || main_loop.expired())
            return;

        auto s = state.lock();

        // If the cursor is already being scaled, do nothing
        if(s->animation_alarm)
            return;

        auto const wind_time = std::chrono::milliseconds{125};
        auto const wind_down_start = wind_time + duration;
        auto const repeat_delay = std::chrono::milliseconds{8};
        auto const start_scale = s->scale;
        auto const end_scale = s->scale * scale_multiplier;
        auto const ml = main_loop.lock();

        s->animation_alarm = ml->create_repeating_alarm(
            [this,
             repeat_delay,
             time = std::chrono::milliseconds{0},
             wind_down_start,
             start_scale,
             end_scale,
             wind_time]() mutable
            {
                auto const scale = [&] -> std::optional<float>
                {
                    if (time < wind_time)
                    {
                        auto const t = static_cast<float>(time.count()) / wind_time.count();
                        return std::lerp(start_scale, end_scale, t);
                    }
                    else if (time > wind_down_start)
                    {
                        auto const t = static_cast<float>((time - wind_down_start).count()) / wind_time.count();
                        return std::lerp(end_scale, start_scale, t);
                    }
                    return std::nullopt;
                }();


                if(scale)
                {
                    this->scale(*scale);
                }

                time += repeat_delay;
            },
            repeat_delay);

        s->animation_stop_alarm = ml->create_alarm(
            [this, start_scale]
            {
                {
                    auto const s = state.lock();
                    s->animation_alarm->cancel();
                    s->animation_alarm = nullptr;
                }

                // Reset scale just in case
                this->scale(start_scale);
            });

        s->animation_alarm->reschedule_in(std::chrono::milliseconds{0});
        auto const animation_duration = wind_time + duration + wind_time;
        s->animation_stop_alarm->reschedule_in(animation_duration);
    }

    float const default_scale;

    struct State
    {
        float scale;
        std::shared_ptr<mir::time::Alarm> animation_alarm{nullptr};
        std::unique_ptr<mir::time::Alarm> animation_stop_alarm{nullptr};
    };

    mir::Synchronised<State> state;

    // accessibility_manager is only updated during the single-threaded initialization phase of startup
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
    std::weak_ptr<mir::MainLoop> main_loop;
};

miral::CursorScale::CursorScale()
    : self{std::make_shared<Self>(1.0)}
{
}

miral::CursorScale::CursorScale(live_config::Store& config_store) : miral::CursorScale{}
{
    config_store.add_float_attribute(
        {"cursor", "scale"},
        "Cursor scale",
       [this](live_config::Key const& key, std::optional<float> val)
        {
            if (val)
            {
                if (*val >= 0.0)
                {
                    scale(*val);
                }
                else
                {
                    mir::log_warning(
                        "Config value %s does not support negative values. Ignoring the supplied value (%f)...",
                        key.to_string().c_str(), *val);
                }
            }
            else
            {
                scale(self->default_scale);
            }
        });
}

miral::CursorScale::CursorScale(float default_scale)
    : self{std::make_shared<Self>(default_scale)}
{
}

miral::CursorScale::~CursorScale() = default;

void miral::CursorScale::scale(float new_scale) const
{
    self->scale(new_scale);
}

void miral::CursorScale::scale_temporarily(float new_scale, std::chrono::milliseconds duration) const
{
    self->scale_temporarily(new_scale, duration);
}

void miral::CursorScale::operator()(mir::Server& server) const
{
    auto const* const cursor_scale_opt = "cursor-scale";
    server.add_configuration_option(
        cursor_scale_opt,
        "Mouse cursor scale, e.g. 2. "
        "Accepts any value in the range [0, 100]",
        self->state.lock()->scale);

    server.add_init_callback(
        [&server, this, cursor_scale_opt]
        {
            auto const& options = server.get_options();

            self->accessibility_manager = server.the_accessibility_manager();
            self->main_loop = server.the_main_loop();
            auto const scale = options->get<double>(cursor_scale_opt);

            if (auto am = self->accessibility_manager.lock())
            {
                self->state.lock()->scale = scale;
                am->cursor_scale(scale);
            }
        });
}
