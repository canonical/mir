/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "InputDevice"

#include <cutils/log.h>
#define DEBUG_PROBE 0

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <androidfw/InputDevice.h>

namespace android {

static const char* CONFIGURATION_FILE_DIR[] = {
        "idc/",
        "keylayout/",
        "keychars/",
};

static const char* CONFIGURATION_FILE_EXTENSION[] = {
        ".idc",
        ".kl",
        ".kcm",
};

static bool isValidNameChar(char ch) {
    return isascii(ch) && (isdigit(ch) || isalpha(ch) || ch == '-' || ch == '_');
}

static void appendInputDeviceConfigurationFileRelativePath(String8& path,
        const String8& name, InputDeviceConfigurationFileType type) {
    path.append(CONFIGURATION_FILE_DIR[type]);
    for (size_t i = 0; i < name.length(); i++) {
        char ch = name[i];
        if (!isValidNameChar(ch)) {
            ch = '_';
        }
        path.append(&ch, 1);
    }
    path.append(CONFIGURATION_FILE_EXTENSION[type]);
}

String8 getInputDeviceConfigurationFilePathByDeviceIdentifier(
        const InputDeviceIdentifier& deviceIdentifier,
        InputDeviceConfigurationFileType type) {
    if (deviceIdentifier.vendor !=0 && deviceIdentifier.product != 0) {
        if (deviceIdentifier.version != 0) {
            // Try vendor product version.
            String8 versionPath(getInputDeviceConfigurationFilePathByName(
                    formatString8("Vendor_%04x_Product_%04x_Version_%04x",
                            deviceIdentifier.vendor, deviceIdentifier.product,
                            deviceIdentifier.version),
                    type));
            if (!isEmpty(versionPath)) {
                return versionPath;
            }
        }

        // Try vendor product.
        String8 productPath(getInputDeviceConfigurationFilePathByName(
                formatString8("Vendor_%04x_Product_%04x",
                        deviceIdentifier.vendor, deviceIdentifier.product),
                type));
        if (!isEmpty(productPath)) {
            return productPath;
        }
    }

    // Try device name.
    return getInputDeviceConfigurationFilePathByName(deviceIdentifier.name, type);
}

String8 getConfigurationFilePathInDirectory(String8 const& directory, String8 const& name, InputDeviceConfigurationFileType type)
{
    String8 path = directory;
    appendInputDeviceConfigurationFileRelativePath(path, name, type);
#if DEBUG_PROBE
    ALOGD("Probing for input device configuration file: path='%s'", path.c_str());
#endif
    if (!access(c_str(path), R_OK)) {
#if DEBUG_PROBE
        ALOGD("Found");
#endif
        return path;
    }

    return String8();
}
    
String8 getInputDeviceConfigurationFilePathByName(
        const String8& name, InputDeviceConfigurationFileType type) {
    // Search system repository.
    String8 path;
    setTo(path, "/usr/share/");
    auto result = getConfigurationFilePathInDirectory(path, name, type);
    if (result.length())
        return result;

    {
        const char *root_env = getenv("ANDROID_ROOT");
        if (root_env == NULL) root_env = "";
        setTo(path, root_env);
    }
    // </mir modifications>
    path.append("/usr/");
    result = getConfigurationFilePathInDirectory(path, name, type);
    if (result.length())
        return result;

    // Search user repository.
    // TODO Should only look here if not in safe mode. ( ?? ~ racarr)
    {
        const char *data_env = getenv("ANDROID_DATA");
        if (data_env == NULL) data_env = "";
        setTo(path, data_env);
    }

    path.append("/system/devices/");
    result = getConfigurationFilePathInDirectory(path, name, type);
    if (result.length())
        return result;

    // Not found.
#if DEBUG_PROBE
    ALOGD("Probe failed to find input device configuration file: name='%s', type=%d",
            name.c_str(), type);
#endif
    return String8();
}


// --- InputDeviceInfo ---

InputDeviceInfo::InputDeviceInfo() {
    initialize(-1, -1, InputDeviceIdentifier(), String8(), false);
}

InputDeviceInfo::InputDeviceInfo(const InputDeviceInfo& other) :
        mId(other.mId), mGeneration(other.mGeneration), mIdentifier(other.mIdentifier),
        mAlias(other.mAlias), mIsExternal(other.mIsExternal), mSources(other.mSources),
        mKeyboardType(other.mKeyboardType),
        mKeyCharacterMap(other.mKeyCharacterMap),
        mHasVibrator(other.mHasVibrator),
        mMotionRanges(other.mMotionRanges) {
}

InputDeviceInfo::~InputDeviceInfo() {
}

void InputDeviceInfo::initialize(int32_t id, int32_t generation,
        const InputDeviceIdentifier& identifier, const String8& alias, bool isExternal) {
    mId = id;
    mGeneration = generation;
    mIdentifier = identifier;
    mAlias = alias;
    mIsExternal = isExternal;
    mSources = 0;
    mKeyboardType = AINPUT_KEYBOARD_TYPE_NONE;
    mHasVibrator = false;
    mMotionRanges.clear();
}

const InputDeviceInfo::MotionRange* InputDeviceInfo::getMotionRange(
        int32_t axis, uint32_t source) const {
    size_t numRanges = mMotionRanges.size();
    for (size_t i = 0; i < numRanges; i++) {
        const MotionRange& range = mMotionRanges.itemAt(i);
        if (range.axis == axis && range.source == source) {
            return &range;
        }
    }
    return NULL;
}

void InputDeviceInfo::addSource(uint32_t source) {
    mSources |= source;
}

void InputDeviceInfo::addMotionRange(int32_t axis, uint32_t source, float min, float max,
        float flat, float fuzz) {
    MotionRange range = { axis, source, min, max, flat, fuzz };
    mMotionRanges.add(range);
}

void InputDeviceInfo::addMotionRange(const MotionRange& range) {
    mMotionRanges.add(range);
}

} // namespace android
