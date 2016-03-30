/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TOOLKIT_EVENTS_KEYMAP_EVENT_H_
#define MIR_TOOLKIT_EVENTS_KEYMAP_EVENT_H_

#include <mir_toolkit/events/event.h>

#include <xkbcommon/xkbcommon.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the new keymap reported by this MirKeymapEvent
 *
 * \deprecated keymap credentials are no longer available use
 * mir_keymap_event_get_keymap_buffer instead.
 *
 * \param[in] ev The keymap event
 * \param[out] rules XKB rules describing the new keymap.
 */
void mir_keymap_event_get_rules(MirKeymapEvent const* ev,
                                struct xkb_rule_names* names)
    __attribute__ ((deprecated));

/**
 * Retrieve the new keymap reported by this MirKeymapEvent
 *
 * The keymap buffer is only valid while the MirKeymapEvent is valid.
 * The buffer can be used via xkb_keymap_new_from_buffer
 * \param[in] ev The keymap event
 * \param[out] buffer the xkbcommon keymap
 * \param[out] length of the buffer
 */
void mir_keymap_event_get_keymap_buffer(MirKeymapEvent const* ev,
                                        char const** buffer, size_t *length);

/**
 * Retrieve the device id the keymap reported by this MirKeymapEvent applies to
 *
 * \param[in] ev The keymap event
 * \param[out] rules XKB rules describing the new keymap.
 */
MirInputDeviceId mir_keymap_event_get_device_id(MirKeymapEvent const* ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_KEYMAP_EVENT_H_ */
