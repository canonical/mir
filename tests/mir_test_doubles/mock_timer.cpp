/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/test/doubles/mock_timer.h"

#include "mir/lockable_callback.h"
#include "mir/basic_callback.h"

#include <mutex>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

class FakeAlarm : public mir::time::Alarm
{
public:
    FakeAlarm(std::shared_ptr<mir::LockableCallback> const& callback,
              std::shared_ptr<mt::FakeClock> const& clock);
    ~FakeAlarm() override;


    bool cancel() override;
    State state() const override;
    bool reschedule_in(std::chrono::milliseconds delay) override;
    bool reschedule_for(mir::time::Timestamp timeout) override;

private:    
    struct InternalState
    {
        explicit InternalState(std::shared_ptr<mir::LockableCallback> const& callback);
        State state;
        std::shared_ptr<mir::LockableCallback> callback;
        mt::FakeClock::time_point threshold;
    };

    bool handle_time_change(InternalState& state, mt::FakeClock::time_point now);

    std::shared_ptr<InternalState> const internal_state;
    std::shared_ptr<mt::FakeClock> const clock;
};

FakeAlarm::InternalState::InternalState(
    std::shared_ptr<mir::LockableCallback> const& callback)
    : state{mir::time::Alarm::pending}, callback{callback}
{
}

FakeAlarm::FakeAlarm(
    std::shared_ptr<mir::LockableCallback> const& callback,
    std::shared_ptr<mt::FakeClock> const& clock)
    : internal_state{std::make_shared<InternalState>(callback)}, clock{clock}
{
}

FakeAlarm::~FakeAlarm()
{
    cancel();
}

bool FakeAlarm::cancel()
{
    if (internal_state->state == triggered)
        return false;

    internal_state->state = cancelled;
    return true;
}

FakeAlarm::State FakeAlarm::state() const
{
    return internal_state->state;
}

bool FakeAlarm::handle_time_change(InternalState& state,
                                   mir::test::FakeClock::time_point now)
{
    if (state.state == pending)
    {
        if (now > state.threshold)
        {
            state.state = triggered;
            auto& handler = *state.callback;
            std::lock_guard<mir::LockableCallback> guard{handler};
            handler();
            return false;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool FakeAlarm::reschedule_in(std::chrono::milliseconds delay)
{
    bool rescheduled = internal_state->state == pending;

    internal_state->state = pending;
    internal_state->threshold = clock->now() + delay;

    std::shared_ptr<InternalState> state_copy{internal_state};
    clock->register_time_change_callback([this, state_copy](mt::FakeClock::time_point now)
    {
        return handle_time_change(*state_copy, now);
    });
    return rescheduled;
}

bool FakeAlarm::reschedule_for(mir::time::Timestamp timeout)
{
    // time::Timestamp is on a different clock, so not directly comparable
    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(timeout.time_since_epoch() -
                                                                       clock->now().time_since_epoch());
    return reschedule_in(delay);
}
}

mtd::FakeTimer::FakeTimer(std::shared_ptr<FakeClock> const& clock) : clock{clock}
{
}

std::unique_ptr<mir::time::Alarm> mir::test::doubles::FakeTimer::create_alarm(std::function<void()> const& callback)
{
    auto const handler = std::make_shared<BasicCallback>(callback);
    return std::unique_ptr<mir::time::Alarm>{new FakeAlarm{handler, clock}};
}

std::unique_ptr<mir::time::Alarm> mir::test::doubles::FakeTimer::create_alarm(std::shared_ptr<LockableCallback> const& callback)
{
    return std::unique_ptr<mir::time::Alarm>{new FakeAlarm{callback, clock}};
}
