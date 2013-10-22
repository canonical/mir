/*
 * Copyright (C) 2013 Canonical LTD
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
 *
 * Author: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

// TODO: ~dandrader. What license should this file be under?

#ifndef INPUT_EVENT_PRINTER_H
#define INPUT_EVENT_PRINTER_H

#include <stdio.h>

void synEvCodeToStr(char *codeStr, int code) {
    switch(code) {
        case SYN_REPORT:
            sprintf(codeStr, "SYN_REPORT");
            break;
        case SYN_CONFIG:
            sprintf(codeStr, "SYN_CONFIG");
            break;
        case SYN_MT_REPORT:
            sprintf(codeStr, "SYN_MT_REPORT");
            break;
        case SYN_DROPPED:
            sprintf(codeStr, "SYN_DROPPED");
            break;
        default:
            sprintf(codeStr, "0x%08x", code);
            break;
    }
}

void absEvCodeToStr(char *codeStr, int code) {
    switch(code) {
        case ABS_X:
            sprintf(codeStr, "ABS_X");
            break;
        case ABS_Y:
            sprintf(codeStr, "ABS_Y");
            break;
        case ABS_MT_TOUCH_MAJOR:
            sprintf(codeStr, "ABS_MT_TOUCH_MAJOR");
            break;
        case ABS_MT_TOUCH_MINOR:
            sprintf(codeStr, "ABS_MT_TOUCH_MINOR");
            break;
        case ABS_MT_ORIENTATION:
            sprintf(codeStr, "ABS_MT_ORIENTATION");
            break;
        case ABS_MT_POSITION_X:
            sprintf(codeStr, "ABS_MT_POSITION_X");
            break;
        case ABS_MT_POSITION_Y:
            sprintf(codeStr, "ABS_MT_POSITION_Y");
            break;
        case ABS_MT_TOOL_TYPE:
            sprintf(codeStr, "ABS_MT_TOOL_TYPE");
            break;
        default:
            sprintf(codeStr, "0x%08x", code);
            break;
    }
}

void keyEvCodeToStr(char *codeStr, int code) {
    switch(code) {
        case BTN_TOUCH:
            sprintf(codeStr, "BTN_TOUCH");
            break;
        case BTN_STYLUS:
            sprintf(codeStr, "BTN_STYLUS");
            break;
        default:
            sprintf(codeStr, "0x%08x", code);
            break;
    }
}

void inputEvToStr(char *buffer, int type, int code, int value) {
    char codeStr[100];
    switch (type) {
    case EV_SYN:
        synEvCodeToStr(codeStr, code);
        sprintf(buffer, "EV_SYN, %s, 0x%08x", codeStr, value);
        break;
    case EV_KEY:
        keyEvCodeToStr(codeStr, code);
        sprintf(buffer, "EV_KEY, %s, 0x%08x", codeStr, value);
        break;
    case EV_REL:
        sprintf(buffer, "EV_REL, 0x%08x, 0x%08x", code, value);
        break;
    case EV_ABS:
        absEvCodeToStr(codeStr, code);
        sprintf(buffer, "EV_ABS, %s, 0x%08x", codeStr, value);
        break;
    case EV_MSC:
        sprintf(buffer, "EV_MSC, 0x%08x, 0x%08x", code, value);
        break;
    case EV_SW:
        sprintf(buffer, "EV_SW, 0x%08x, 0x%08x", code, value);
        break;
    case EV_LED:
        sprintf(buffer, "EV_LED, 0x%08x, 0x%08x", code, value);
        break;
    case EV_SND:
        sprintf(buffer, "EV_SND, 0x%08x, 0x%08x", code, value);
        break;
    case EV_REP:
        sprintf(buffer, "EV_REP, 0x%08x, 0x%08x", code, value);
        break;
    case EV_FF:
        sprintf(buffer, "EV_FF, 0x%08x, 0x%08x", code, value);
        break;
    case EV_PWR:
        sprintf(buffer, "EV_PWR, 0x%08x, 0x%08x", code, value);
        break;
    case EV_FF_STATUS:
        sprintf(buffer, "EV_FF_STATUS, 0x%08x, 0x%08x", code, value);
        break;
    default:
        sprintf(buffer, "0x%08x, 0x%08x, 0x%08x", type, code, value);
        break;
    }
}

#endif // INPUT_EVENT_PRINTER_H
