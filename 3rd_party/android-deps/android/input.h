/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ANDROID_INPUT_H
#define _ANDROID_INPUT_H

/******************************************************************
 *
 * IMPORTANT NOTICE:
 *
 *   This file is part of Android's set of stable system headers
 *   exposed by the Android NDK (Native Development Kit).
 *
 *   Third-party source AND binary code relies on the definitions
 *   here to be FROZEN ON ALL UPCOMING PLATFORM RELEASES.
 *
 *   - DO NOT MODIFY ENUMS (EXCEPT IF YOU ADD NEW 32-BIT VALUES)
 *   - DO NOT MODIFY CONSTANTS OR FUNCTIONAL MACROS
 *   - DO NOT CHANGE THE SIGNATURE OF FUNCTIONS IN ANY WAY
 *   - DO NOT CHANGE THE LAYOUT OR SIZE OF STRUCTURES
 */

/*
 * Structures and functions to receive and process input events in
 * native code.
 *
 * NOTE: These functions MUST be implemented by /system/lib/libui.so
 */

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Key states (may be returned by queries about the current state of a
 * particular key code, scan code or switch).
 */
enum {
    /* The key state is unknown or the requested key itself is not supported. */
    AKEY_STATE_UNKNOWN = -1,

    /* The key is up. */
    AKEY_STATE_UP = 0,

    /* The key is down. */
    AKEY_STATE_DOWN = 1,

    /* The key is down but is a virtual key press that is being emulated by the system. */
    AKEY_STATE_VIRTUAL = 2
};

/*
 * Meta key / modifer state.
 */
enum {
    /* No meta keys are pressed. */
    AMETA_NONE = 0,

    /* This mask is used to check whether one of the ALT meta keys is pressed. */
    AMETA_ALT_ON = 0x02,

    /* This mask is used to check whether the left ALT meta key is pressed. */
    AMETA_ALT_LEFT_ON = 0x10,

    /* This mask is used to check whether the right ALT meta key is pressed. */
    AMETA_ALT_RIGHT_ON = 0x20,

    /* This mask is used to check whether one of the SHIFT meta keys is pressed. */
    AMETA_SHIFT_ON = 0x01,

    /* This mask is used to check whether the left SHIFT meta key is pressed. */
    AMETA_SHIFT_LEFT_ON = 0x40,

    /* This mask is used to check whether the right SHIFT meta key is pressed. */
    AMETA_SHIFT_RIGHT_ON = 0x80,

    /* This mask is used to check whether the SYM meta key is pressed. */
    AMETA_SYM_ON = 0x04,

    /* This mask is used to check whether the FUNCTION meta key is pressed. */
    AMETA_FUNCTION_ON = 0x08,

    /* This mask is used to check whether one of the CTRL meta keys is pressed. */
    AMETA_CTRL_ON = 0x1000,

    /* This mask is used to check whether the left CTRL meta key is pressed. */
    AMETA_CTRL_LEFT_ON = 0x2000,

    /* This mask is used to check whether the right CTRL meta key is pressed. */
    AMETA_CTRL_RIGHT_ON = 0x4000,

    /* This mask is used to check whether one of the META meta keys is pressed. */
    AMETA_META_ON = 0x10000,

    /* This mask is used to check whether the left META meta key is pressed. */
    AMETA_META_LEFT_ON = 0x20000,

    /* This mask is used to check whether the right META meta key is pressed. */
    AMETA_META_RIGHT_ON = 0x40000,

    /* This mask is used to check whether the CAPS LOCK meta key is on. */
    AMETA_CAPS_LOCK_ON = 0x100000,

    /* This mask is used to check whether the NUM LOCK meta key is on. */
    AMETA_NUM_LOCK_ON = 0x200000,

    /* This mask is used to check whether the SCROLL LOCK meta key is on. */
    AMETA_SCROLL_LOCK_ON = 0x400000
};

/*
 * Input events.
 *
 * Input events are opaque structures.  Use the provided accessors functions to
 * read their properties.
 */
struct AInputEvent;
typedef struct AInputEvent AInputEvent;

/*
 * Input event types.
 */
enum {
    /* Indicates that the input event is a key event. */
    AINPUT_EVENT_TYPE_KEY = 1,

    /* Indicates that the input event is a motion event. */
    AINPUT_EVENT_TYPE_MOTION = 2
};

/*
 * Key event actions.
 */
enum {
    /* The key has been pressed down. */
    AKEY_EVENT_ACTION_DOWN = 0,

    /* The key has been released. */
    AKEY_EVENT_ACTION_UP = 1,

    /* Multiple duplicate key events have occurred in a row, or a complex string is
     * being delivered.  The repeat_count property of the key event contains the number
     * of times the given key code should be executed.
     */
    AKEY_EVENT_ACTION_MULTIPLE = 2
};

/*
 * Key event flags.
 */
enum {
    /* This mask is set if the device woke because of this key event. */
    AKEY_EVENT_FLAG_WOKE_HERE = 0x1,

    /* This mask is set if the key event was generated by a software keyboard. */
    AKEY_EVENT_FLAG_SOFT_KEYBOARD = 0x2,

    /* This mask is set if we don't want the key event to cause us to leave touch mode. */
    AKEY_EVENT_FLAG_KEEP_TOUCH_MODE = 0x4,

    /* This mask is set if an event was known to come from a trusted part
     * of the system.  That is, the event is known to come from the user,
     * and could not have been spoofed by a third party component. */
    AKEY_EVENT_FLAG_FROM_SYSTEM = 0x8,

    /* This mask is used for compatibility, to identify enter keys that are
     * coming from an IME whose enter key has been auto-labelled "next" or
     * "done".  This allows TextView to dispatch these as normal enter keys
     * for old applications, but still do the appropriate action when
     * receiving them. */
    AKEY_EVENT_FLAG_EDITOR_ACTION = 0x10,

    /* When associated with up key events, this indicates that the key press
     * has been canceled.  Typically this is used with virtual touch screen
     * keys, where the user can slide from the virtual key area on to the
     * display: in that case, the application will receive a canceled up
     * event and should not perform the action normally associated with the
     * key.  Note that for this to work, the application can not perform an
     * action for a key until it receives an up or the long press timeout has
     * expired. */
    AKEY_EVENT_FLAG_CANCELED = 0x20,

    /* This key event was generated by a virtual (on-screen) hard key area.
     * Typically this is an area of the touchscreen, outside of the regular
     * display, dedicated to "hardware" buttons. */
    AKEY_EVENT_FLAG_VIRTUAL_HARD_KEY = 0x40,

    /* This flag is set for the first key repeat that occurs after the
     * long press timeout. */
    AKEY_EVENT_FLAG_LONG_PRESS = 0x80,

    /* Set when a key event has AKEY_EVENT_FLAG_CANCELED set because a long
     * press action was executed while it was down. */
    AKEY_EVENT_FLAG_CANCELED_LONG_PRESS = 0x100,

    /* Set for AKEY_EVENT_ACTION_UP when this event's key code is still being
     * tracked from its initial down.  That is, somebody requested that tracking
     * started on the key down and a long press has not caused
     * the tracking to be canceled. */
    AKEY_EVENT_FLAG_TRACKING = 0x200,

    /* Set when a key event has been synthesized to implement default behavior
     * for an event that the application did not handle.
     * Fallback key events are generated by unhandled trackball motions
     * (to emulate a directional keypad) and by certain unhandled key presses
     * that are declared in the key map (such as special function numeric keypad
     * keys when numlock is off). */
    AKEY_EVENT_FLAG_FALLBACK = 0x400
};

/*
 * Motion event actions.
 */

/* Bit shift for the action bits holding the pointer index as
 * defined by AMOTION_EVENT_ACTION_POINTER_INDEX_MASK.
 */
#define AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT 8

enum {
    /* Bit mask of the parts of the action code that are the action itself.
     */
    AMOTION_EVENT_ACTION_MASK = 0xff,

    /* Bits in the action code that represent a pointer index, used with
     * AMOTION_EVENT_ACTION_POINTER_DOWN and AMOTION_EVENT_ACTION_POINTER_UP.  Shifting
     * down by AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT provides the actual pointer
     * index where the data for the pointer going up or down can be found.
     */
    AMOTION_EVENT_ACTION_POINTER_INDEX_MASK  = 0xff00,

    /* A pressed gesture has started, the motion contains the initial starting location.
     */
    AMOTION_EVENT_ACTION_DOWN = 0,

    /* A pressed gesture has finished, the motion contains the final release location
     * as well as any intermediate points since the last down or move event.
     */
    AMOTION_EVENT_ACTION_UP = 1,

    /* A change has happened during a press gesture (between AMOTION_EVENT_ACTION_DOWN and
     * AMOTION_EVENT_ACTION_UP).  The motion contains the most recent point, as well as
     * any intermediate points since the last down or move event.
     */
    AMOTION_EVENT_ACTION_MOVE = 2,

    /* The current gesture has been aborted.
     * You will not receive any more points in it.  You should treat this as
     * an up event, but not perform any action that you normally would.
     */
    AMOTION_EVENT_ACTION_CANCEL = 3,

    /* A movement has happened outside of the normal bounds of the UI element.
     * This does not provide a full gesture, but only the initial location of the movement/touch.
     */
    AMOTION_EVENT_ACTION_OUTSIDE = 4,

    /* A non-primary pointer has gone down.
     * The bits in AMOTION_EVENT_ACTION_POINTER_INDEX_MASK indicate which pointer changed.
     */
    AMOTION_EVENT_ACTION_POINTER_DOWN = 5,

    /* A non-primary pointer has gone up.
     * The bits in AMOTION_EVENT_ACTION_POINTER_INDEX_MASK indicate which pointer changed.
     */
    AMOTION_EVENT_ACTION_POINTER_UP = 6,

    /* A change happened but the pointer is not down (unlike AMOTION_EVENT_ACTION_MOVE).
     * The motion contains the most recent point, as well as any intermediate points since
     * the last hover move event.
     */
    AMOTION_EVENT_ACTION_HOVER_MOVE = 7,

    /* The motion event contains relative vertical and/or horizontal scroll offsets.
     * Use getAxisValue to retrieve the information from AMOTION_EVENT_AXIS_VSCROLL
     * and AMOTION_EVENT_AXIS_HSCROLL.
     * The pointer may or may not be down when this event is dispatched.
     * This action is always delivered to the winder under the pointer, which
     * may not be the window currently touched.
     */
    AMOTION_EVENT_ACTION_SCROLL = 8,

    /* The pointer is not down but has entered the boundaries of a window or view.
     */
    AMOTION_EVENT_ACTION_HOVER_ENTER = 9,

    /* The pointer is not down but has exited the boundaries of a window or view.
     */
    AMOTION_EVENT_ACTION_HOVER_EXIT = 10
};

/*
 * Motion event flags.
 */
enum {
    /* This flag indicates that the window that received this motion event is partly
     * or wholly obscured by another visible window above it.  This flag is set to true
     * even if the event did not directly pass through the obscured area.
     * A security sensitive application can check this flag to identify situations in which
     * a malicious application may have covered up part of its content for the purpose
     * of misleading the user or hijacking touches.  An appropriate response might be
     * to drop the suspect touches or to take additional precautions to confirm the user's
     * actual intent.
     */
    AMOTION_EVENT_FLAG_WINDOW_IS_OBSCURED = 0x1
};

/*
 * Motion event edge touch flags.
 */
enum {
    /* No edges intersected */
    AMOTION_EVENT_EDGE_FLAG_NONE = 0,

    /* Flag indicating the motion event intersected the top edge of the screen. */
    AMOTION_EVENT_EDGE_FLAG_TOP = 0x01,

    /* Flag indicating the motion event intersected the bottom edge of the screen. */
    AMOTION_EVENT_EDGE_FLAG_BOTTOM = 0x02,

    /* Flag indicating the motion event intersected the left edge of the screen. */
    AMOTION_EVENT_EDGE_FLAG_LEFT = 0x04,

    /* Flag indicating the motion event intersected the right edge of the screen. */
    AMOTION_EVENT_EDGE_FLAG_RIGHT = 0x08
};

/*
 * Constants that identify each individual axis of a motion event.
 * Refer to the documentation on the MotionEvent class for descriptions of each axis.
 */
enum {
    AMOTION_EVENT_AXIS_X = 0,
    AMOTION_EVENT_AXIS_Y = 1,
    AMOTION_EVENT_AXIS_PRESSURE = 2,
    AMOTION_EVENT_AXIS_SIZE = 3,
    AMOTION_EVENT_AXIS_TOUCH_MAJOR = 4,
    AMOTION_EVENT_AXIS_TOUCH_MINOR = 5,
    AMOTION_EVENT_AXIS_TOOL_MAJOR = 6,
    AMOTION_EVENT_AXIS_TOOL_MINOR = 7,
    AMOTION_EVENT_AXIS_ORIENTATION = 8,
    AMOTION_EVENT_AXIS_VSCROLL = 9,
    AMOTION_EVENT_AXIS_HSCROLL = 10,
    AMOTION_EVENT_AXIS_Z = 11,
    AMOTION_EVENT_AXIS_RX = 12,
    AMOTION_EVENT_AXIS_RY = 13,
    AMOTION_EVENT_AXIS_RZ = 14,
    AMOTION_EVENT_AXIS_HAT_X = 15,
    AMOTION_EVENT_AXIS_HAT_Y = 16,
    AMOTION_EVENT_AXIS_LTRIGGER = 17,
    AMOTION_EVENT_AXIS_RTRIGGER = 18,
    AMOTION_EVENT_AXIS_THROTTLE = 19,
    AMOTION_EVENT_AXIS_RUDDER = 20,
    AMOTION_EVENT_AXIS_WHEEL = 21,
    AMOTION_EVENT_AXIS_GAS = 22,
    AMOTION_EVENT_AXIS_BRAKE = 23,
    AMOTION_EVENT_AXIS_DISTANCE = 24,
    AMOTION_EVENT_AXIS_TILT = 25,
    AMOTION_EVENT_AXIS_GENERIC_1 = 32,
    AMOTION_EVENT_AXIS_GENERIC_2 = 33,
    AMOTION_EVENT_AXIS_GENERIC_3 = 34,
    AMOTION_EVENT_AXIS_GENERIC_4 = 35,
    AMOTION_EVENT_AXIS_GENERIC_5 = 36,
    AMOTION_EVENT_AXIS_GENERIC_6 = 37,
    AMOTION_EVENT_AXIS_GENERIC_7 = 38,
    AMOTION_EVENT_AXIS_GENERIC_8 = 39,
    AMOTION_EVENT_AXIS_GENERIC_9 = 40,
    AMOTION_EVENT_AXIS_GENERIC_10 = 41,
    AMOTION_EVENT_AXIS_GENERIC_11 = 42,
    AMOTION_EVENT_AXIS_GENERIC_12 = 43,
    AMOTION_EVENT_AXIS_GENERIC_13 = 44,
    AMOTION_EVENT_AXIS_GENERIC_14 = 45,
    AMOTION_EVENT_AXIS_GENERIC_15 = 46,
    AMOTION_EVENT_AXIS_GENERIC_16 = 47

    // NOTE: If you add a new axis here you must also add it to several other files.
    //       Refer to frameworks/base/core/java/android/view/MotionEvent.java for the full list.
};

/*
 * Constants that identify buttons that are associated with motion events.
 * Refer to the documentation on the MotionEvent class for descriptions of each button.
 */
enum {
    AMOTION_EVENT_BUTTON_PRIMARY = 1 << 0,
    AMOTION_EVENT_BUTTON_SECONDARY = 1 << 1,
    AMOTION_EVENT_BUTTON_TERTIARY = 1 << 2,
    AMOTION_EVENT_BUTTON_BACK = 1 << 3,
    AMOTION_EVENT_BUTTON_FORWARD = 1 << 4
};

/*
 * Constants that identify tool types.
 * Refer to the documentation on the MotionEvent class for descriptions of each tool type.
 */
enum {
    AMOTION_EVENT_TOOL_TYPE_UNKNOWN = 0,
    AMOTION_EVENT_TOOL_TYPE_FINGER = 1,
    AMOTION_EVENT_TOOL_TYPE_STYLUS = 2,
    AMOTION_EVENT_TOOL_TYPE_MOUSE = 3,
    AMOTION_EVENT_TOOL_TYPE_ERASER = 4
};

/*
 * Input sources.
 *
 * Refer to the documentation on android.view.InputDevice for more details about input sources
 * and their correct interpretation.
 */
enum {
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010
};

enum {
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,

    AINPUT_SOURCE_ANY = 0xffffff00
};

/*
 * Keyboard types.
 *
 * Refer to the documentation on android.view.InputDevice for more details.
 */
enum {
    AINPUT_KEYBOARD_TYPE_NONE = 0,
    AINPUT_KEYBOARD_TYPE_NON_ALPHABETIC = 1,
    AINPUT_KEYBOARD_TYPE_ALPHABETIC = 2
};

#ifdef __cplusplus
}
#endif

#endif // _ANDROID_INPUT_H
