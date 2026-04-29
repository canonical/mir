#ifndef MIR_COMMON_SWITCH_EVENT_H_
#define MIR_COMMON_SWITCH_EVENT_H_

#include <mir/events/input_event.h>

struct MirSwitchEvent : MirInputEvent
{
    MirSwitchEvent();

    auto clone() const -> MirEvent* override;

    MirSwitchAction action() const;
    void set_action(MirSwitchAction action);

    MirSwitchState state() const;
    void set_state(MirSwitchState state);

private:
    MirSwitchAction action_ = {};
    MirSwitchState state_ = {};
};

#endif
