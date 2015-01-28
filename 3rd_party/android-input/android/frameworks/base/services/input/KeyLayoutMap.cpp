/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "KeyLayoutMap"

#include <stdlib.h>
#include <android/keycodes.h>
#include <androidfw/Keyboard.h>
#include <androidfw/KeyLayoutMap.h>
#include <std/Log.h>
#include <std/Errors.h>
#include <std/Tokenizer.h>
#include <std/Timers.h>

// Enables debug output for the parser.
#define DEBUG_PARSER 0

// Enables debug output for parser performance.
#define DEBUG_PARSER_PERFORMANCE 0

// Enables debug output for mapping.
#define DEBUG_MAPPING 0


namespace android {

static const char* WHITESPACE = " \t\r";

// --- KeyLayoutMap ---

KeyLayoutMap::KeyLayoutMap() {
}

KeyLayoutMap::~KeyLayoutMap() {
}

status_t KeyLayoutMap::load(Tokenizer *tokenizer, sp<KeyLayoutMap>* outMap) {
    status_t status = 0;
    outMap->clear();

    sp<KeyLayoutMap> map = new KeyLayoutMap();
    if (!map.get()) {
	ALOGE("Error allocating key layout map.");
	status = NO_MEMORY;
    } else {
#if DEBUG_PARSER_PERFORMANCE
	std::chrono::nanoseconds startTime = systemTime(SYSTEM_TIME_MONOTONIC);
#endif
	Parser parser(map.get(), tokenizer);
	status = parser.parse();
#if DEBUG_PARSER_PERFORMANCE
	std::chrono::nanoseconds elapsedTime = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;
	ALOGD("Parsed key layout map file '%s' %d lines in %0.3fms.",
	      tokenizer->getFilename().string(), tokenizer->getLineNumber(),
	      elapsedTime / 1000000.0);
#endif
	if (!status) {
	    *outMap = map;
	}
    }
    return status;
}

status_t KeyLayoutMap::load(const String8& filename, sp<KeyLayoutMap>* outMap) {
    outMap->clear();

    Tokenizer* tokenizer;
    status_t status = Tokenizer::open(filename, &tokenizer);
    if (status) {
        ALOGE("Error %d opening key layout map file %s.", status, c_str(filename));
    } else {
	status = load(tokenizer, outMap);
        delete tokenizer;
    }
    return status;
}
status_t KeyLayoutMap::load(const String8& filename, const char* contents, sp<KeyLayoutMap>* outMap) {
    outMap->clear();

    Tokenizer* tokenizer;
    status_t status = Tokenizer::fromContents(filename, contents, &tokenizer);
    if (status) {
        ALOGE("Error %d opening key layout map file %s.", status, c_str(filename));
    } else {
	status = load(tokenizer, outMap);
        delete tokenizer;
    }
    return status;
}

status_t KeyLayoutMap::mapKey(int32_t scanCode, int32_t usageCode,
        int32_t* outKeyCode, uint32_t* outFlags) const {
    const Key* key = getKey(scanCode, usageCode);
    if (!key) {
#if DEBUG_MAPPING
        ALOGD("mapKey: scanCode=%d, usageCode=0x%08x ~ Failed.", scanCode, usageCode);
#endif
        *outKeyCode = AKEYCODE_UNKNOWN;
        *outFlags = 0;
        return NAME_NOT_FOUND;
    }

    *outKeyCode = key->keyCode;
    *outFlags = key->flags;

#if DEBUG_MAPPING
    ALOGD("mapKey: scanCode=%d, usageCode=0x%08x ~ Result keyCode=%d, outFlags=0x%08x.",
            scanCode, usageCode, *outKeyCode, *outFlags);
#endif
    return NO_ERROR;
}

const KeyLayoutMap::Key* KeyLayoutMap::getKey(int32_t scanCode, int32_t usageCode) const {
    if (usageCode) {
        ssize_t index = mKeysByUsageCode.indexOfKey(usageCode);
        if (index >= 0) {
            return &mKeysByUsageCode.valueAt(index);
        }
    }
    if (scanCode) {
        ssize_t index = mKeysByScanCode.indexOfKey(scanCode);
        if (index >= 0) {
            return &mKeysByScanCode.valueAt(index);
        }
    }
    return NULL;
}

status_t KeyLayoutMap::findScanCodesForKey(int32_t keyCode, Vector<int32_t>* outScanCodes) const {
    const size_t N = mKeysByScanCode.size();
    for (size_t i=0; i<N; i++) {
        if (mKeysByScanCode.valueAt(i).keyCode == keyCode) {
            outScanCodes->add(mKeysByScanCode.keyAt(i));
        }
    }
    return NO_ERROR;
}

status_t KeyLayoutMap::mapAxis(int32_t scanCode, AxisInfo* outAxisInfo) const {
    ssize_t index = mAxes.indexOfKey(scanCode);
    if (index < 0) {
#if DEBUG_MAPPING
        ALOGD("mapAxis: scanCode=%d ~ Failed.", scanCode);
#endif
        return NAME_NOT_FOUND;
    }

    *outAxisInfo = mAxes.valueAt(index);

#if DEBUG_MAPPING
    ALOGD("mapAxis: scanCode=%d ~ Result mode=%d, axis=%d, highAxis=%d, "
            "splitValue=%d, flatOverride=%d.",
            scanCode,
            outAxisInfo->mode, outAxisInfo->axis, outAxisInfo->highAxis,
            outAxisInfo->splitValue, outAxisInfo->flatOverride);
#endif
    return NO_ERROR;
}


// --- KeyLayoutMap::Parser ---

KeyLayoutMap::Parser::Parser(KeyLayoutMap* map, Tokenizer* tokenizer) :
        mMap(map), mTokenizer(tokenizer) {
}

KeyLayoutMap::Parser::~Parser() {
}

status_t KeyLayoutMap::Parser::parse() {
    while (!mTokenizer->isEof()) {
#if DEBUG_PARSER
        ALOGD("Parsing %s: '%s'.", c_str(mTokenizer->getLocation()),
                mTokenizer->peekRemainderOfLine().string());
#endif

        mTokenizer->skipDelimiters(WHITESPACE);

        if (!mTokenizer->isEol() && mTokenizer->peekChar() != '#') {
            String8 keywordToken = mTokenizer->nextToken(WHITESPACE);
            if (keywordToken == "key") {
                mTokenizer->skipDelimiters(WHITESPACE);
                status_t status = parseKey();
                if (status) return status;
            } else if (keywordToken == "axis") {
                mTokenizer->skipDelimiters(WHITESPACE);
                status_t status = parseAxis();
                if (status) return status;
            } else {
                ALOGE("%s: Expected keyword, got '%s'.", c_str(mTokenizer->getLocation()),
                        c_str(keywordToken));
                return BAD_VALUE;
            }

            mTokenizer->skipDelimiters(WHITESPACE);
            if (!mTokenizer->isEol() && mTokenizer->peekChar() != '#') {
                ALOGE("%s: Expected end of line or trailing comment, got '%s'.",
                        c_str(mTokenizer->getLocation()),
                        c_str(mTokenizer->peekRemainderOfLine()));
                return BAD_VALUE;
            }
        }

        mTokenizer->nextLine();
    }
    return NO_ERROR;
}

status_t KeyLayoutMap::Parser::parseKey() {
    String8 codeToken = mTokenizer->nextToken(WHITESPACE);
    bool mapUsage = false;
    if (codeToken == "usage") {
        mapUsage = true;
        mTokenizer->skipDelimiters(WHITESPACE);
        codeToken = mTokenizer->nextToken(WHITESPACE);
    }

    char* end;
    int32_t code = int32_t(strtol(c_str(codeToken), &end, 0));
    if (*end) {
        ALOGE("%s: Expected key %s number, got '%s'.", c_str(mTokenizer->getLocation()),
                mapUsage ? "usage" : "scan code", c_str(codeToken));
        return BAD_VALUE;
    }
    KeyedVector<int32_t, Key>& map =
            mapUsage ? mMap->mKeysByUsageCode : mMap->mKeysByScanCode;
    if (map.indexOfKey(code) >= 0) {
        ALOGE("%s: Duplicate entry for key %s '%s'.", c_str(mTokenizer->getLocation()),
                mapUsage ? "usage" : "scan code", c_str(codeToken));
        return BAD_VALUE;
    }

    mTokenizer->skipDelimiters(WHITESPACE);
    String8 keyCodeToken = mTokenizer->nextToken(WHITESPACE);
    int32_t keyCode = getKeyCodeByLabel(c_str(keyCodeToken));
    if (!keyCode) {
        ALOGE("%s: Expected key code label, got '%s'.", c_str(mTokenizer->getLocation()),
                c_str(keyCodeToken));
        return BAD_VALUE;
    }

    uint32_t flags = 0;
    for (;;) {
        mTokenizer->skipDelimiters(WHITESPACE);
        if (mTokenizer->isEol() || mTokenizer->peekChar() == '#') break;

        String8 flagToken = mTokenizer->nextToken(WHITESPACE);
        uint32_t flag = getKeyFlagByLabel(c_str(flagToken));
        if (!flag) {
            ALOGE("%s: Expected key flag label, got '%s'.", c_str(mTokenizer->getLocation()),
                    c_str(flagToken));
            return BAD_VALUE;
        }
        if (flags & flag) {
            ALOGE("%s: Duplicate key flag '%s'.", c_str(mTokenizer->getLocation()),
                    c_str(flagToken));
            return BAD_VALUE;
        }
        flags |= flag;
    }

#if DEBUG_PARSER
    ALOGD("Parsed key %s: code=%d, keyCode=%d, flags=0x%08x.",
            mapUsage ? "usage" : "scan code", code, keyCode, flags);
#endif
    Key key;
    key.keyCode = keyCode;
    key.flags = flags;
    map.add(code, key);
    return NO_ERROR;
}

status_t KeyLayoutMap::Parser::parseAxis() {
    String8 scanCodeToken = mTokenizer->nextToken(WHITESPACE);
    char* end;
    int32_t scanCode = int32_t(strtol(c_str(scanCodeToken), &end, 0));
    if (*end) {
        ALOGE("%s: Expected axis scan code number, got '%s'.", c_str(mTokenizer->getLocation()),
                c_str(scanCodeToken));
        return BAD_VALUE;
    }
    if (mMap->mAxes.indexOfKey(scanCode) >= 0) {
        ALOGE("%s: Duplicate entry for axis scan code '%s'.", c_str(mTokenizer->getLocation()),
                c_str(scanCodeToken));
        return BAD_VALUE;
    }

    AxisInfo axisInfo;

    mTokenizer->skipDelimiters(WHITESPACE);
    String8 token = mTokenizer->nextToken(WHITESPACE);
    if (token == "invert") {
        axisInfo.mode = AxisInfo::MODE_INVERT;

        mTokenizer->skipDelimiters(WHITESPACE);
        String8 axisToken = mTokenizer->nextToken(WHITESPACE);
        axisInfo.axis = getAxisByLabel(c_str(axisToken));
        if (axisInfo.axis < 0) {
            ALOGE("%s: Expected inverted axis label, got '%s'.",
                    c_str(mTokenizer->getLocation()), c_str(axisToken));
            return BAD_VALUE;
        }
    } else if (token == "split") {
        axisInfo.mode = AxisInfo::MODE_SPLIT;

        mTokenizer->skipDelimiters(WHITESPACE);
        String8 splitToken = mTokenizer->nextToken(WHITESPACE);
        axisInfo.splitValue = int32_t(strtol(c_str(splitToken), &end, 0));
        if (*end) {
            ALOGE("%s: Expected split value, got '%s'.",
                    c_str(mTokenizer->getLocation()), c_str(splitToken));
            return BAD_VALUE;
        }

        mTokenizer->skipDelimiters(WHITESPACE);
        String8 lowAxisToken = mTokenizer->nextToken(WHITESPACE);
        axisInfo.axis = getAxisByLabel(c_str(lowAxisToken));
        if (axisInfo.axis < 0) {
            ALOGE("%s: Expected low axis label, got '%s'.",
                    c_str(mTokenizer->getLocation()), c_str(lowAxisToken));
            return BAD_VALUE;
        }

        mTokenizer->skipDelimiters(WHITESPACE);
        String8 highAxisToken = mTokenizer->nextToken(WHITESPACE);
        axisInfo.highAxis = getAxisByLabel(c_str(highAxisToken));
        if (axisInfo.highAxis < 0) {
            ALOGE("%s: Expected high axis label, got '%s'.",
                    c_str(mTokenizer->getLocation()), c_str(highAxisToken));
            return BAD_VALUE;
        }
    } else {
        axisInfo.axis = getAxisByLabel(c_str(token));
        if (axisInfo.axis < 0) {
            ALOGE("%s: Expected axis label, 'split' or 'invert', got '%s'.",
                    c_str(mTokenizer->getLocation()), c_str(token));
            return BAD_VALUE;
        }
    }

    for (;;) {
        mTokenizer->skipDelimiters(WHITESPACE);
        if (mTokenizer->isEol() || mTokenizer->peekChar() == '#') {
            break;
        }
        String8 keywordToken = mTokenizer->nextToken(WHITESPACE);
        if (keywordToken == "flat") {
            mTokenizer->skipDelimiters(WHITESPACE);
            String8 flatToken = mTokenizer->nextToken(WHITESPACE);
            axisInfo.flatOverride = int32_t(strtol(c_str(flatToken), &end, 0));
            if (*end) {
                ALOGE("%s: Expected flat value, got '%s'.",
                        c_str(mTokenizer->getLocation()), c_str(flatToken));
                return BAD_VALUE;
            }
        } else {
            ALOGE("%s: Expected keyword 'flat', got '%s'.",
                    c_str(mTokenizer->getLocation()), c_str(keywordToken));
            return BAD_VALUE;
        }
    }

#if DEBUG_PARSER
    ALOGD("Parsed axis: scanCode=%d, mode=%d, axis=%d, highAxis=%d, "
            "splitValue=%d, flatOverride=%d.",
            scanCode,
            axisInfo.mode, axisInfo.axis, axisInfo.highAxis,
            axisInfo.splitValue, axisInfo.flatOverride);
#endif
    mMap->mAxes.add(scanCode, axisInfo);
    return NO_ERROR;
}

};
