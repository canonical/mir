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

#define LOG_TAG "PointerController"

//#define LOG_NDEBUG 0

// Log debug messages about pointer updates
#define DEBUG_POINTER_UPDATES 0

#include "PointerController.h"

#include <cutils/log.h>

namespace android {

// --- PointerController ---

PointerController::PointerController() {

    AutoMutex _l(mLock);

    mLocked.displayWidth = -1;
    mLocked.displayHeight = -1;
    mLocked.displayOrientation = DISPLAY_ORIENTATION_0;

    mLocked.presentation = PRESENTATION_POINTER;
    mLocked.presentationChanged = false;

    mLocked.pointerX = 0;
    mLocked.pointerY = 0;
    mLocked.pointerAlpha = 0.0f; // pointer is initially faded

    mLocked.buttonState = 0;
}

PointerController::~PointerController() {
    AutoMutex _l(mLock);

}

bool PointerController::getBounds(float* outMinX, float* outMinY,
        float* outMaxX, float* outMaxY) const {
    AutoMutex _l(mLock);

    return getBoundsLocked(outMinX, outMinY, outMaxX, outMaxY);
}

bool PointerController::getBoundsLocked(float* outMinX, float* outMinY,
        float* outMaxX, float* outMaxY) const {
    if (mLocked.displayWidth <= 0 || mLocked.displayHeight <= 0) {
        return false;
    }

    *outMinX = 0;
    *outMinY = 0;
    switch (mLocked.displayOrientation) {
    case DISPLAY_ORIENTATION_90:
    case DISPLAY_ORIENTATION_270:
        *outMaxX = mLocked.displayHeight - 1;
        *outMaxY = mLocked.displayWidth - 1;
        break;
    default:
        *outMaxX = mLocked.displayWidth - 1;
        *outMaxY = mLocked.displayHeight - 1;
        break;
    }
    return true;
}

void PointerController::move(float deltaX, float deltaY) {
#if DEBUG_POINTER_UPDATES
    ALOGD("Move pointer by deltaX=%0.3f, deltaY=%0.3f", deltaX, deltaY);
#endif
    if (deltaX == 0.0f && deltaY == 0.0f) {
        return;
    }

    AutoMutex _l(mLock);

    setPositionLocked(mLocked.pointerX + deltaX, mLocked.pointerY + deltaY);
}

void PointerController::setButtonState(int32_t buttonState) {
#if DEBUG_POINTER_UPDATES
    ALOGD("Set button state 0x%08x", buttonState);
#endif
    AutoMutex _l(mLock);

    if (mLocked.buttonState != buttonState) {
        mLocked.buttonState = buttonState;
    }
}

int32_t PointerController::getButtonState() const {
    AutoMutex _l(mLock);

    return mLocked.buttonState;
}

void PointerController::setPosition(float x, float y) {
#if DEBUG_POINTER_UPDATES
    ALOGD("Set pointer position to x=%0.3f, y=%0.3f", x, y);
#endif
    AutoMutex _l(mLock);

    setPositionLocked(x, y);
}

void PointerController::setPositionLocked(float x, float y) {
    float minX, minY, maxX, maxY;
    if (getBoundsLocked(&minX, &minY, &maxX, &maxY)) {
        if (x <= minX) {
            mLocked.pointerX = minX;
        } else if (x >= maxX) {
            mLocked.pointerX = maxX;
        } else {
            mLocked.pointerX = x;
        }
        if (y <= minY) {
            mLocked.pointerY = minY;
        } else if (y >= maxY) {
            mLocked.pointerY = maxY;
        } else {
            mLocked.pointerY = y;
        }
        updatePointerLocked();
    }
}

void PointerController::getPosition(float* outX, float* outY) const {
    AutoMutex _l(mLock);

    *outX = mLocked.pointerX;
    *outY = mLocked.pointerY;
}

void PointerController::fade(Transition transition) {
  // TODO: Implement
}

void PointerController::unfade(Transition transition) {
  // TODO: Implement
}

void PointerController::setPresentation(Presentation presentation) {
    AutoMutex _l(mLock);

    if (mLocked.presentation != presentation) {
        mLocked.presentation = presentation;
        mLocked.presentationChanged = true;

        updatePointerLocked();
    }
}

void PointerController::setSpots(const PointerCoords* spotCoords, uint32_t spotCount) {
#if DEBUG_POINTER_UPDATES
    ALOGD("setSpots: spotCount=%d", spotCount);
    for (size_t i = 0; i < spotCount; ++i) {
        const PointerCoords& c = spotCoords[i];
        ALOGD(" spot %d: position=(%0.3f, %0.3f), pressure=%0.3f", i,
                c.getAxisValue(AMOTION_EVENT_AXIS_X),
                c.getAxisValue(AMOTION_EVENT_AXIS_Y),
                c.getAxisValue(AMOTION_EVENT_AXIS_PRESSURE));
    }
#endif

    AutoMutex _l(mLock);

    //TODO: Implement
}

void PointerController::clearSpots() {
#if DEBUG_POINTER_UPDATES
    ALOGD("clearSpots");
#endif

    AutoMutex _l(mLock);
}

void PointerController::setDisplaySize(int32_t width, int32_t height) {
    AutoMutex _l(mLock);

    if (mLocked.displayWidth != width || mLocked.displayHeight != height) {
        mLocked.displayWidth = width;
        mLocked.displayHeight = height;

        float minX, minY, maxX, maxY;
        if (getBoundsLocked(&minX, &minY, &maxX, &maxY)) {
            mLocked.pointerX = (minX + maxX) * 0.5f;
            mLocked.pointerY = (minY + maxY) * 0.5f;
        } else {
            mLocked.pointerX = 0;
            mLocked.pointerY = 0;
        }

        updatePointerLocked();
    }
}

void PointerController::setDisplayOrientation(int32_t orientation) {
    AutoMutex _l(mLock);

    if (mLocked.displayOrientation != orientation) {
        // Apply offsets to convert from the pixel top-left corner position to the pixel center.
        // This creates an invariant frame of reference that we can easily rotate when
        // taking into account that the pointer may be located at fractional pixel offsets.
        float x = mLocked.pointerX + 0.5f;
        float y = mLocked.pointerY + 0.5f;
        float temp;

        // Undo the previous rotation.
        switch (mLocked.displayOrientation) {
        case DISPLAY_ORIENTATION_90:
            temp = x;
            x = mLocked.displayWidth - y;
            y = temp;
            break;
        case DISPLAY_ORIENTATION_180:
            x = mLocked.displayWidth - x;
            y = mLocked.displayHeight - y;
            break;
        case DISPLAY_ORIENTATION_270:
            temp = x;
            x = y;
            y = mLocked.displayHeight - temp;
            break;
        }

        // Perform the new rotation.
        switch (orientation) {
        case DISPLAY_ORIENTATION_90:
            temp = x;
            x = y;
            y = mLocked.displayWidth - temp;
            break;
        case DISPLAY_ORIENTATION_180:
            x = mLocked.displayWidth - x;
            y = mLocked.displayHeight - y;
            break;
        case DISPLAY_ORIENTATION_270:
            temp = x;
            x = mLocked.displayHeight - y;
            y = temp;
            break;
        }

        // Apply offsets to convert from the pixel center to the pixel top-left corner position
        // and save the results.
        mLocked.pointerX = x - 0.5f;
        mLocked.pointerY = y - 0.5f;
        mLocked.displayOrientation = orientation;

        updatePointerLocked();
    }
}

void PointerController::updatePointerLocked() {
  // TODO: Implement
}

} // namespace android
