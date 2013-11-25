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

#ifndef _UI_POINTER_CONTROLLER_H
#define _UI_POINTER_CONTROLLER_H

#include <androidfw/Input.h>
#include <std/Mutex.h>
#include <std/RefBase.h>
#include <std/String8.h>

namespace android {

enum {
    DISPLAY_ORIENTATION_0 = 0,
    DISPLAY_ORIENTATION_90 = 1,
    DISPLAY_ORIENTATION_180 = 2,
    DISPLAY_ORIENTATION_270 = 3
};



/**
 * Interface for tracking a mouse / touch pad pointer and touch pad spots.
 *
 * The spots are sprites on screen that visually represent the positions of
 * fingers
 *
 * The pointer controller is responsible for providing synchronization and for tracking
 * display orientation changes if needed.
 */
class PointerControllerInterface : public virtual RefBase {
protected:
    PointerControllerInterface() { }
    virtual ~PointerControllerInterface() { }

public:
    /* Gets the bounds of the region that the pointer can traverse.
     * Returns true if the bounds are available. */
    virtual bool getBounds(float* outMinX, float* outMinY,
            float* outMaxX, float* outMaxY) const = 0;

    /* Move the pointer. */
    virtual void move(float deltaX, float deltaY) = 0;

    /* Sets a mask that indicates which buttons are pressed. */
    virtual void setButtonState(int32_t buttonState) = 0;

    /* Gets a mask that indicates which buttons are pressed. */
    virtual int32_t getButtonState() const = 0;

    /* Sets the absolute location of the pointer. */
    virtual void setPosition(float x, float y) = 0;

    /* Gets the absolute location of the pointer. */
    virtual void getPosition(float* outX, float* outY) const = 0;

    enum Transition {
        // Fade/unfade immediately.
        TRANSITION_IMMEDIATE,
        // Fade/unfade gradually.
        TRANSITION_GRADUAL
    };

    /* Fades the pointer out now. */
    virtual void fade(Transition transition) = 0;

    /* Makes the pointer visible if it has faded out.
     * The pointer never unfades itself automatically.  This method must be called
     * by the client whenever the pointer is moved or a button is pressed and it
     * wants to ensure that the pointer becomes visible again. */
    virtual void unfade(Transition transition) = 0;

    enum Presentation {
        // Show the mouse pointer.
        PRESENTATION_POINTER,
        // Show spots and a spot anchor in place of the mouse pointer.
        PRESENTATION_SPOT
    };

    /* Sets the mode of the pointer controller. */
    virtual void setPresentation(Presentation presentation) = 0;

    /* Sets the spots for the current gesture.
     * The spots are not subject to the inactivity timeout like the pointer
     * itself it since they are expected to remain visible for so long as
     * the fingers are on the touch pad.
     *
     * The values of the AMOTION_EVENT_AXIS_PRESSURE axis is significant.
     * For spotCoords, pressure != 0 indicates that the spot's location is being
     * pressed (not hovering).
     */
    virtual void setSpots(const PointerCoords* spotCoords, uint32_t spotCount) = 0;

    /* Removes all spots. */
    virtual void clearSpots() = 0;
};

/*
 * Tracks pointer movements and draws the pointer sprite to a surface.
 *
 * Handles pointer acceleration and animation.
 */
class PointerController : public PointerControllerInterface/*, public MessageHandler*/ {
protected:
    virtual ~PointerController();

public:
    enum InactivityTimeout {
        INACTIVITY_TIMEOUT_NORMAL = 0,
        INACTIVITY_TIMEOUT_SHORT = 1
    };

    PointerController();

    virtual bool getBounds(float* outMinX, float* outMinY,
            float* outMaxX, float* outMaxY) const;
    virtual void move(float deltaX, float deltaY);
    virtual void setButtonState(int32_t buttonState);
    virtual int32_t getButtonState() const;
    virtual void setPosition(float x, float y);
    virtual void getPosition(float* outX, float* outY) const;
    virtual void fade(Transition transition);
    virtual void unfade(Transition transition);

    virtual void setPresentation(Presentation presentation);
    virtual void setSpots(const PointerCoords* spotCoords, uint32_t spotCount);
    virtual void clearSpots();

    void setDisplaySize(int32_t width, int32_t height);
    void setDisplayOrientation(int32_t orientation);

private:
    mutable Mutex mLock;

    struct Locked {
        int32_t displayWidth;
        int32_t displayHeight;
        int32_t displayOrientation;

        Presentation presentation;
        bool presentationChanged;

        float pointerX;
        float pointerY;
        float pointerAlpha;

        int32_t buttonState;
    } mLocked;

    bool getBoundsLocked(float* outMinX, float* outMinY, float* outMaxX, float* outMaxY) const;
    void setPositionLocked(float x, float y);

    void updatePointerLocked();
};

} // namespace android

#endif // _UI_POINTER_CONTROLLER_H
