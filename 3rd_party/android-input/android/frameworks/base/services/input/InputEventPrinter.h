/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

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
        case ABS_MT_SLOT:
            sprintf(codeStr, "ABS_MT_SLOT");
            break;
        case ABS_MT_TOUCH_MAJOR:
            sprintf(codeStr, "ABS_MT_TOUCH_MAJOR");
            break;
        case ABS_MT_TOUCH_MINOR:
            sprintf(codeStr, "ABS_MT_TOUCH_MINOR");
            break;
        case ABS_MT_WIDTH_MAJOR:
            sprintf(codeStr, "ABS_MT_WIDTH_MAJOR");
            break;
        case ABS_MT_WIDTH_MINOR:
            sprintf(codeStr, "ABS_MT_WIDTH_MINOR");
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
        case ABS_MT_BLOB_ID:
            sprintf(codeStr, "ABS_MT_BLOB_ID");
            break;
        case ABS_MT_TRACKING_ID:
            sprintf(codeStr, "ABS_MT_TRACKING_ID");
            break;
        case ABS_MT_PRESSURE:
            sprintf(codeStr, "ABS_MT_PRESSURE");
            break;
        case ABS_MT_DISTANCE:
            sprintf(codeStr, "ABS_MT_DISTANCE");
            break;
        default:
            sprintf(codeStr, "0x%08x", code);
            break;
    }
}

void keyEvCodeToStr(char *codeStr, int code) {
    switch(code) {
        case BTN_TOOL_PEN:
            sprintf(codeStr, "BTN_TOOL_PEN");
            break;
        case BTN_TOOL_FINGER:
            sprintf(codeStr, "BTN_TOOL_FINGER");
            break;
        case BTN_TOOL_MOUSE:
            sprintf(codeStr, "BTN_TOOL_MOUSE");
            break;
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
