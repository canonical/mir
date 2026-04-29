#ifndef MIR_TOOLKIT_SWITCH_EVENT_H_
#define MIR_TOOLKIT_SWITCH_EVENT_H_

#include <mir_toolkit/events/input/input_event.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MirSwitchEvent MirSwitchEvent;

MirSwitchAction mir_switch_event_action(MirSwitchEvent const* event);
MirSwitchState mir_switch_event_state(MirSwitchEvent const* event);

#ifdef __cplusplus
}
#endif

#endif /* MIR_TOOLKIT_SWITCH_EVENT_H_ */
