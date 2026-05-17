#include <mir/events/input_event.h>
#include <mir/events/switch_event.h>

MirSwitchEvent::MirSwitchEvent(MirInputDeviceId device_id) :
    MirInputEvent{mir_input_event_type_switch}
{
    set_device_id(device_id);
}

MirEvent* MirSwitchEvent::clone() const
{
    return new MirSwitchEvent{*this};
}

MirSwitchAction MirSwitchEvent::action() const
{
    return action_;
}

void MirSwitchEvent::set_action(MirSwitchAction action)
{
    action_ = action;
}

MirSwitchState MirSwitchEvent::state() const
{
    return state_;
}

void MirSwitchEvent::set_state(MirSwitchState state)
{
    state_ = state;
}
