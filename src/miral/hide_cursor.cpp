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

#include <miral/hide_cursor.h>
#include <miral/live_config.h>

#include <mir/input/cursor_hiding_policy.h>
#include <mir/server.h>
#include <mir/synchronised.h>

#include <algorithm>
#include <optional>

namespace mi = mir::input;

namespace
{
auto const default_inactivity_timeout = std::chrono::milliseconds{5000};
auto const default_reveal_threshold = 10.0f;
}

struct miral::HideCursor::Self
{
    struct State
    {
        bool on_touch{false};
        bool on_keyboard{false};
        bool on_inactivity{false};
        std::chrono::milliseconds inactivity_timeout{default_inactivity_timeout};
        float reveal_threshold{default_reveal_threshold};
        std::weak_ptr<mi::CursorHidingPolicy> policy;

        void apply() const
        {
            auto const policy = this->policy.lock();
            if (!policy)
                return;

            policy->hide_on_touch(on_touch);
            policy->hide_on_keyboard(on_keyboard);
            policy->hide_after_inactivity(on_inactivity ? std::optional{inactivity_timeout} : std::nullopt);
            policy->reveal_threshold(reveal_threshold);
        }
    };

    template<typename Update>
    void update(Update&& update)
    {
        auto const s = state.lock();
        update(*s);
        s->apply();
    }

    mir::Synchronised<State> state;
};

miral::HideCursor::HideCursor()
    : self{std::make_shared<Self>()}
{
}

miral::HideCursor::HideCursor(live_config::Store& config_store) : HideCursor{}
{
    config_store.add_bool_attribute(
        {"cursor", "hide_on_touch"},
        "Hide the cursor on touch",
        [self=self](live_config::Key const&, std::optional<bool> value)
        {
            self->update([&](auto& s) { s.on_touch = value.value_or(false); });
        });

    config_store.add_bool_attribute(
        {"cursor", "hide_on_keyboard"},
        "Hide the cursor on keyboard input",
        [self=self](live_config::Key const&, std::optional<bool> value)
        {
            self->update([&](auto& s) { s.on_keyboard = value.value_or(false); });
        });

    config_store.add_bool_attribute(
        {"cursor", "hide_on_inactivity"},
        "Hide the cursor after pointer inactivity",
        [self=self](live_config::Key const&, std::optional<bool> value)
        {
            self->update([&](auto& s) { s.on_inactivity = value.value_or(false); });
        });

    config_store.add_int_attribute(
        {"cursor", "inactivity_timeout_ms"},
        "Pointer inactivity timeout in milliseconds",
        [self=self](live_config::Key const&, std::optional<int> value)
        {
            auto const timeout = value ? std::chrono::milliseconds{std::max(*value, 0)} : default_inactivity_timeout;
            self->update([&](auto& s) { s.inactivity_timeout = timeout; });
        });

    config_store.add_float_attribute(
        {"cursor", "reveal_threshold"},
        "Pointer motion needed to reveal a hidden cursor",
        [self=self](live_config::Key const&, std::optional<float> value)
        {
            auto const threshold = std::max(value.value_or(default_reveal_threshold), 0.0f);
            self->update([&](auto& s) { s.reveal_threshold = threshold; });
        });
}

miral::HideCursor::~HideCursor() = default;

auto miral::HideCursor::on_touch(bool enable) -> HideCursor&
{
    self->update([&](auto& s) { s.on_touch = enable; });
    return *this;
}

auto miral::HideCursor::on_keyboard(bool enable) -> HideCursor&
{
    self->update([&](auto& s) { s.on_keyboard = enable; });
    return *this;
}

auto miral::HideCursor::on_inactivity(bool enable) -> HideCursor&
{
    self->update([&](auto& s) { s.on_inactivity = enable; });
    return *this;
}

auto miral::HideCursor::inactivity_timeout(std::chrono::milliseconds timeout) -> HideCursor&
{
    self->update([&](auto& s) { s.inactivity_timeout = std::max(timeout, std::chrono::milliseconds{0}); });
    return *this;
}

auto miral::HideCursor::reveal_threshold(float logical_pixels) -> HideCursor&
{
    self->update([&](auto& s) { s.reveal_threshold = std::max(logical_pixels, 0.0f); });
    return *this;
}

void miral::HideCursor::operator()(mir::Server& server) const
{
    server.add_init_callback(
        [self=self, &server]
        {
            self->update([&](auto& s) { s.policy = server.the_cursor_hiding_policy(); });
        });
}
