// Copyright (C) 2010 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <androidfw/GenericKeyMap.h>

const char* android::GenericKeyMap::key_layout_contents()
{
    static const char* result = 
            "key 1     ESCAPE\n"                \
            "key 2     1\n"                     \
            "key 3     2\n"                     \
            "key 4     3\n"                     \
            "key 5     4\n"                     \
            "key 6     5\n"                     \
            "key 7     6\n"                     \
            "key 8     7\n"                     \
            "key 9     8\n"                     \
            "key 10    9\n"                     \
            "key 11    0\n"                     \
            "key 12    MINUS\n"                 \
            "key 13    EQUALS\n"                \
            "key 14    DEL\n"                   \
            "key 15    TAB\n"                   \
            "key 16    Q\n"                     \
            "key 17    W\n"                     \
            "key 18    E\n"                     \
            "key 19    R\n"                     \
            "key 20    T\n"                     \
            "key 21    Y\n"                     \
            "key 22    U\n"                     \
            "key 23    I\n"                     \
            "key 24    O\n"                     \
            "key 25    P\n"                     \
            "key 26    LEFT_BRACKET\n"          \
            "key 27    RIGHT_BRACKET\n"         \
            "key 28    ENTER\n"                 \
            "key 29    CTRL_LEFT\n"             \
            "key 30    A\n"                     \
            "key 31    S\n"                     \
            "key 32    D\n"                     \
            "key 33    F\n"                     \
            "key 34    G\n" \
            "key 35    H\n" \
            "key 36    J\n" \
            "key 37    K\n" \
            "key 38    L\n" \
            "key 39    SEMICOLON\n" \
            "key 40    APOSTROPHE\n" \
            "key 41    GRAVE\n" \
            "key 42    SHIFT_LEFT\n" \
            "key 43    BACKSLASH\n" \
            "key 44    Z\n" \
            "key 45    X\n" \
            "key 46    C\n" \
            "key 47    V\n" \
            "key 48    B\n" \
            "key 49    N\n" \
            "key 50    M\n" \
            "key 51    COMMA\n" \
            "key 52    PERIOD\n" \
            "key 53    SLASH\n" \
            "key 54    SHIFT_RIGHT\n" \
            "key 55    NUMPAD_MULTIPLY\n" \
            "key 56    ALT_LEFT\n" \
            "key 57    SPACE\n" \
            "key 58    CAPS_LOCK\n" \
            "key 59    F1\n" \
            "key 60    F2\n" \
            "key 61    F3\n" \
            "key 62    F4\n" \
            "key 63    F5\n" \
            "key 64    F6\n" \
            "key 65    F7\n" \
            "key 66    F8\n" \
            "key 67    F9\n" \
            "key 68    F10\n" \
            "key 69    NUM_LOCK\n" \
            "key 70    SCROLL_LOCK\n" \
            "key 71    NUMPAD_7\n" \
            "key 72    NUMPAD_8\n" \
            "key 73    NUMPAD_9\n" \
            "key 74    NUMPAD_SUBTRACT\n" \
            "key 75    NUMPAD_4\n" \
            "key 76    NUMPAD_5\n" \
            "key 77    NUMPAD_6\n" \
            "key 78    NUMPAD_ADD\n" \
            "key 79    NUMPAD_1\n" \
            "key 80    NUMPAD_2\n" \
            "key 81    NUMPAD_3\n" \
            "key 82    NUMPAD_0\n" \
            "key 83    NUMPAD_DOT\n" \
            "# key 84 (undefined)\n" \
            "key 85    ZENKAKU_HANKAKU\n" \
            "key 86    BACKSLASH\n" \
            "key 87    F11\n" \
            "key 88    F12\n" \
            "key 89    RO\n" \
            "# key 90 \"KEY_KATAKANA\"\n" \
            "# key 91 \"KEY_HIRAGANA\"\n" \
            "key 92    HENKAN\n" \
            "key 93    KATAKANA_HIRAGANA\n" \
            "key 94    MUHENKAN\n" \
            "key 95    NUMPAD_COMMA\n" \
            "key 96    NUMPAD_ENTER\n" \
            "key 97    CTRL_RIGHT\n" \
            "key 98    NUMPAD_DIVIDE\n" \
            "key 99    SYSRQ\n" \
            "key 100   ALT_RIGHT\n" \
            "# key 101 \"KEY_LINEFEED\"\n" \
            "key 102   MOVE_HOME\n" \
            "key 103   DPAD_UP\n" \
            "key 104   PAGE_UP\n" \
            "key 105   DPAD_LEFT\n" \
            "key 106   DPAD_RIGHT\n" \
            "key 107   MOVE_END\n" \
            "key 108   DPAD_DOWN\n" \
            "key 109   PAGE_DOWN\n" \
            "key 110   INSERT\n" \
            "key 111   FORWARD_DEL\n" \
            "# key 112 \"KEY_MACRO\"\n" \
            "key 113   VOLUME_MUTE\n" \
            "key 114   VOLUME_DOWN\n" \
            "key 115   VOLUME_UP\n" \
            "key 116   POWER             WAKE\n" \
            "key 117   NUMPAD_EQUALS\n" \
            "# key 118 \"KEY_KPPLUSMINUS\"\n" \
            "key 119   BREAK\n" \
            "# key 120 (undefined)\n" \
            "key 121   NUMPAD_COMMA\n" \
            "key 122   KANA\n" \
            "key 123   EISU\n" \
            "key 124   YEN\n" \
            "key 125   META_LEFT\n" \
            "key 126   META_RIGHT\n" \
            "key 127   MENU              WAKE_DROPPED\n" \
            "key 128   MEDIA_STOP\n" \
            "# key 129 \"KEY_AGAIN\"\n" \
            "# key 130 \"KEY_PROPS\"\n" \
            "# key 131 \"KEY_UNDO\"\n" \
            "# key 132 \"KEY_FRONT\"\n" \
            "# key 133 \"KEY_COPY\"\n" \
            "# key 134 \"KEY_OPEN\"\n" \
            "# key 135 \"KEY_PASTE\"\n" \
            "# key 136 \"KEY_FIND\"\n" \
            "# key 137 \"KEY_CUT\"\n" \
            "# key 138 \"KEY_HELP\"\n" \
            "key 139   MENU              WAKE_DROPPED\n" \
            "key 140   CALCULATOR\n" \
            "# key 141 \"KEY_SETUP\"\n" \
            "key 142   POWER             WAKE\n" \
            "key 143   POWER             WAKE\n" \
            "# key 144 \"KEY_FILE\"\n" \
            "# key 145 \"KEY_SENDFILE\"\n" \
            "# key 146 \"KEY_DELETEFILE\"\n" \
            "# key 147 \"KEY_XFER\"\n" \
            "# key 148 \"KEY_PROG1\"\n" \
            "# key 149 \"KEY_PROG2\"\n" \
            "key 150   EXPLORER\n" \
            "# key 151 \"KEY_MSDOS\"\n" \
            "key 152   POWER             WAKE\n" \
            "# key 153 \"KEY_DIRECTION\"\n" \
            "# key 154 \"KEY_CYCLEWINDOWS\"\n" \
            "key 155   ENVELOPE\n" \
            "key 156   BOOKMARK\n" \
            "# key 157 \"KEY_COMPUTER\"\n" \
            "key 158   BACK              WAKE_DROPPED\n" \
            "key 159   FORWARD\n" \
            "key 160   MEDIA_CLOSE\n" \
            "key 161   MEDIA_EJECT\n" \
            "key 162   MEDIA_EJECT\n" \
            "key 163   MEDIA_NEXT\n" \
            "key 164   MEDIA_PLAY_PAUSE\n" \
            "key 165   MEDIA_PREVIOUS\n" \
            "key 166   MEDIA_STOP\n" \
            "key 167   MEDIA_RECORD\n" \
            "key 168   MEDIA_REWIND\n" \
            "key 169   CALL\n" \
            "# key 170 \"KEY_ISO\"\n" \
            "key 171   MUSIC\n" \
            "key 172   HOME\n" \
            "# key 173 \"KEY_REFRESH\"\n" \
            "# key 174 \"KEY_EXIT\"\n" \
            "# key 175 \"KEY_MOVE\"\n" \
            "# key 176 \"KEY_EDIT\"\n" \
            "key 177   PAGE_UP\n" \
            "key 178   PAGE_DOWN\n" \
            "key 179   NUMPAD_LEFT_PAREN\n" \
            "key 180   NUMPAD_RIGHT_PAREN\n" \
            "# key 181 \"KEY_NEW\"\n" \
            "# key 182 \"KEY_REDO\"\n" \
            "# key 183   F13\n" \
            "# key 184   F14\n" \
            "# key 185   F15\n" \
            "# key 186   F16\n" \
            "# key 187   F17\n" \
            "# key 188   F18\n" \
            "# key 189   F19\n" \
            "# key 190   F20\n" \
            "# key 191   F21\n" \
            "# key 192   F22\n" \
            "# key 193   F23\n" \
            "# key 194   F24\n" \
            "# key 195 (undefined)\n" \
            "# key 196 (undefined)\n" \
            "# key 197 (undefined)\n" \
            "# key 198 (undefined)\n" \
            "# key 199 (undefined)\n" \
            "key 200   MEDIA_PLAY\n" \
            "key 201   MEDIA_PAUSE\n" \
            "# key 202 \"KEY_PROG3\"\n" \
            "# key 203 \"KEY_PROG4\"\n" \
            "# key 204 (undefined)\n" \
            "# key 205 \"KEY_SUSPEND\"\n" \
            "# key 206 \"KEY_CLOSE\"\n" \
            "key 207   MEDIA_PLAY\n" \
            "key 208   MEDIA_FAST_FORWARD\n" \
            "# key 209 \"KEY_BASSBOOST\"\n" \
            "# key 210 \"KEY_PRINT\"\n" \
            "# key 211 \"KEY_HP\"\n" \
            "key 212   CAMERA\n" \
            "key 213   MUSIC\n" \
            "# key 214 \"KEY_QUESTION\"\n" \
            "key 215   ENVELOPE\n" \
            "# key 216 \"KEY_CHAT\"\n" \
            "key 217   SEARCH\n" \
            "# key 218 \"KEY_CONNECT\"\n" \
            "# key 219 \"KEY_FINANCE\"\n" \
            "# key 220 \"KEY_SPORT\"\n" \
            "# key 221 \"KEY_SHOP\"\n" \
            "# key 222 \"KEY_ALTERASE\"\n" \
            "# key 223 \"KEY_CANCEL\"\n" \
            "# key 224 \"KEY_BRIGHTNESSDOWN\"\n" \
            "# key 225 \"KEY_BRIGHTNESSUP\"\n" \
            "key 226   HEADSETHOOK\n" \
            "\n" \
            "key 256   BUTTON_1\n" \
            "key 257   BUTTON_2\n" \
            "key 258   BUTTON_3\n" \
            "key 259   BUTTON_4\n" \
            "key 260   BUTTON_5\n" \
            "key 261   BUTTON_6\n" \
            "key 262   BUTTON_7\n" \
            "key 263   BUTTON_8\n" \
            "key 264   BUTTON_9\n" \
            "key 265   BUTTON_10\n" \
            "key 266   BUTTON_11\n" \
            "key 267   BUTTON_12\n" \
            "key 268   BUTTON_13\n" \
            "key 269   BUTTON_14\n" \
            "key 270   BUTTON_15\n" \
            "key 271   BUTTON_16\n" \
            "\n" \
            "key 288   BUTTON_1\n" \
            "key 289   BUTTON_2\n" \
            "key 290   BUTTON_3\n" \
            "key 291   BUTTON_4\n" \
            "key 292   BUTTON_5\n" \
            "key 293   BUTTON_6\n" \
            "key 294   BUTTON_7\n" \
            "key 295   BUTTON_8\n" \
            "key 296   BUTTON_9\n" \
            "key 297   BUTTON_10\n" \
            "key 298   BUTTON_11\n" \
            "key 299   BUTTON_12\n" \
            "key 300   BUTTON_13\n" \
            "key 301   BUTTON_14\n" \
            "key 302   BUTTON_15\n" \
            "key 303   BUTTON_16\n" \
            "\n" \
            "\n" \
            "key 304   BUTTON_A\n" \
            "key 305   BUTTON_B\n" \
            "key 306   BUTTON_C\n" \
            "key 307   BUTTON_X\n" \
            "key 308   BUTTON_Y\n" \
            "key 309   BUTTON_Z\n" \
            "key 310   BUTTON_L1\n" \
            "key 311   BUTTON_R1\n" \
            "key 312   BUTTON_L2\n" \
            "key 313   BUTTON_R2\n" \
            "key 314   BUTTON_SELECT\n" \
            "key 315   BUTTON_START\n" \
            "key 316   BUTTON_MODE\n" \
            "key 317   BUTTON_THUMBL\n" \
            "key 318   BUTTON_THUMBR\n" \
            "\n" \
            "\n" \
            "# key 352 \"KEY_OK\"\n" \
            "key 353   DPAD_CENTER\n" \
            "# key 354 \"KEY_GOTO\"\n" \
            "# key 355 \"KEY_CLEAR\"\n" \
            "# key 356 \"KEY_POWER2\"\n" \
            "# key 357 \"KEY_OPTION\"\n" \
            "# key 358 \"KEY_INFO\"\n" \
            "# key 359 \"KEY_TIME\"\n" \
            "# key 360 \"KEY_VENDOR\"\n" \
            "# key 361 \"KEY_ARCHIVE\"\n" \
            "key 362   GUIDE\n" \
            "# key 363 \"KEY_CHANNEL\"\n" \
            "# key 364 \"KEY_FAVORITES\"\n" \
            "# key 365 \"KEY_EPG\"\n" \
            "key 366   DVR\n" \
            "# key 367 \"KEY_MHP\"\n" \
            "# key 368 \"KEY_LANGUAGE\"\n" \
            "# key 369 \"KEY_TITLE\"\n" \
            "# key 370 \"KEY_SUBTITLE\"\n" \
            "# key 371 \"KEY_ANGLE\"\n" \
            "# key 372 \"KEY_ZOOM\"\n" \
            "# key 373 \"KEY_MODE\"\n" \
            "# key 374 \"KEY_KEYBOARD\"\n" \
            "# key 375 \"KEY_SCREEN\"\n" \
            "# key 376 \"KEY_PC\"\n" \
            "key 377   TV\n" \
            "# key 378 \"KEY_TV2\"\n" \
            "# key 379 \"KEY_VCR\"\n" \
            "# key 380 \"KEY_VCR2\"\n" \
            "# key 381 \"KEY_SAT\"\n" \
            "# key 382 \"KEY_SAT2\"\n" \
            "# key 383 \"KEY_CD\"\n" \
            "# key 384 \"KEY_TAPE\"\n" \
            "# key 385 \"KEY_RADIO\"\n" \
            "# key 386 \"KEY_TUNER\"\n" \
            "# key 387 \"KEY_PLAYER\"\n" \
            "# key 388 \"KEY_TEXT\"\n" \
            "# key 389 \"KEY_DVD\"\n" \
            "# key 390 \"KEY_AUX\"\n" \
            "# key 391 \"KEY_MP3\"\n" \
            "# key 392 \"KEY_AUDIO\"\n" \
            "# key 393 \"KEY_VIDEO\"\n" \
            "# key 394 \"KEY_DIRECTORY\"\n" \
            "# key 395 \"KEY_LIST\"\n" \
            "# key 396 \"KEY_MEMO\"\n" \
            "key 397   CALENDAR\n" \
            "# key 398 \"KEY_RED\"\n" \
            "# key 399 \"KEY_GREEN\"\n" \
            "# key 400 \"KEY_YELLOW\"\n" \
            "# key 401 \"KEY_BLUE\"\n" \
            "key 402   CHANNEL_UP\n" \
            "key 403   CHANNEL_DOWN\n" \
            "# key 404 \"KEY_FIRST\"\n" \
            "# key 405 \"KEY_LAST\"\n" \
            "# key 406 \"KEY_AB\"\n" \
            "# key 407 \"KEY_NEXT\"\n" \
            "# key 408 \"KEY_RESTART\"\n" \
            "# key 409 \"KEY_SLOW\"\n" \
            "# key 410 \"KEY_SHUFFLE\"\n" \
            "# key 411 \"KEY_BREAK\"\n" \
            "# key 412 \"KEY_PREVIOUS\"\n" \
            "# key 413 \"KEY_DIGITS\"\n" \
            "# key 414 \"KEY_TEEN\"\n" \
            "# key 415 \"KEY_TWEN\"\n" \
            "key 429   CONTACTS\n" \
            "\n" \
            "# key 448 \"KEY_DEL_EOL\"\n" \
            "# key 449 \"KEY_DEL_EOS\"\n" \
            "# key 450 \"KEY_INS_LINE\"\n" \
            "# key 451 \"KEY_DEL_LINE\"\n" \
            "\n" \
            "\n" \
            "key 464   FUNCTION\n" \
            "key 465   ESCAPE            FUNCTION\n" \
            "key 466   F1                FUNCTION\n" \
            "key 467   F2                FUNCTION\n" \
            "key 468   F3                FUNCTION\n" \
            "key 469   F4                FUNCTION\n" \
            "key 470   F5                FUNCTION\n" \
            "key 471   F6                FUNCTION\n" \
            "key 472   F7                FUNCTION\n" \
            "key 473   F8                FUNCTION\n" \
            "key 474   F9                FUNCTION\n" \
            "key 475   F10               FUNCTION\n" \
            "key 476   F11               FUNCTION\n" \
            "key 477   F12               FUNCTION\n" \
            "key 478   1                 FUNCTION\n" \
            "key 479   2                 FUNCTION\n" \
            "key 480   D                 FUNCTION\n" \
            "key 481   E                 FUNCTION\n" \
            "key 482   F                 FUNCTION\n" \
            "key 483   S                 FUNCTION\n" \
            "key 484   B                 FUNCTION\n" \
            "\n" \
            "\n" \
            "# key 497 KEY_BRL_DOT1\n" \
            "# key 498 KEY_BRL_DOT2\n" \
            "# key 499 KEY_BRL_DOT3\n" \
            "# key 500 KEY_BRL_DOT4\n" \
            "# key 501 KEY_BRL_DOT5\n" \
            "# key 502 KEY_BRL_DOT6\n" \
            "# key 503 KEY_BRL_DOT7\n" \
            "# key 504 KEY_BRL_DOT8\n" \
            "\n" \
            "\n" \
            "# Joystick and game controller axes.\n" \
            "# Axes that are not mapped will be assigned generic axis numbers by the input subsystem.\n" \
            "axis 0x00 X\n" \
            "axis 0x01 Y\n" \
            "axis 0x02 Z\n" \
            "axis 0x03 RX\n" \
            "axis 0x04 RY\n" \
            "axis 0x05 RZ\n" \
            "axis 0x06 THROTTLE\n" \
            "axis 0x07 RUDDER\n" \
            "axis 0x08 WHEEL\n" \
            "axis 0x09 GAS\n" \
            "axis 0x0a BRAKE\n" \
            "axis 0x10 HAT_X\n" \
            "axis 0x11 HAT_Y\n";
    return result;
}

const char* android::GenericKeyMap::keymap_contents()
{
    static const char* result = 
            "type FULL\n"                       \
            "\n" \
            "### Basic QWERTY keys ###\n" \
            "\n" \
            "key A {\n" \
            "    label:                              'A'\n" \
            "    base:                               'a'\n" \
            "    shift, capslock:                    'A'\n" \
            "}\n" \
            "\n" \
            "key B {\n" \
            "    label:                              'B'\n" \
            "    base:                               'b'\n" \
            "    shift, capslock:                    'B'\n" \
            "}\n" \
            "\n" \
            "key C {\n" \
            "    label:                              'C'\n" \
            "    base:                               'c'\n" \
            "    shift, capslock:                    'C'\n" \
            "    alt:                                '\\u00e7'\n" \
            "    shift+alt:                          '\\u00c7'\n" \
            "}\n" \
            "\n" \
            "key D {\n" \
            "    label:                              'D'\n" \
            "    base:                               'd'\n" \
            "    shift, capslock:                    'D'\n" \
            "}\n" \
            "\n" \
            "key E {\n" \
            "    label:                              'E'\n" \
            "    base:                               'e'\n" \
            "    shift, capslock:                    'E'\n" \
            "    alt:                                '\\u0301'\n" \
            "}\n" \
            "\n" \
            "key F {\n" \
            "    label:                              'F'\n" \
            "    base:                               'f'\n" \
            "    shift, capslock:                    'F'\n" \
            "}\n" \
            "\n" \
            "key G {\n" \
            "    label:                              'G'\n" \
            "    base:                               'g'\n" \
            "    shift, capslock:                    'G'\n" \
            "}\n" \
            "\n" \
            "key H {\n" \
            "    label:                              'H'\n" \
            "    base:                               'h'\n" \
            "    shift, capslock:                    'H'\n" \
            "}\n" \
            "\n" \
            "key I {\n" \
            "    label:                              'I'\n" \
            "    base:                               'i'\n" \
            "    shift, capslock:                    'I'\n" \
            "    alt:                                '\\u0302'\n" \
            "}\n" \
            "\n" \
            "key J {\n" \
            "    label:                              'J'\n" \
            "    base:                               'j'\n" \
            "    shift, capslock:                    'J'\n" \
            "}\n" \
            "\n" \
            "key K {\n" \
            "    label:                              'K'\n" \
            "    base:                               'k'\n" \
            "    shift, capslock:                    'K'\n" \
            "}\n" \
            "\n" \
            "key L {\n" \
            "    label:                              'L'\n" \
            "    base:                               'l'\n" \
            "    shift, capslock:                    'L'\n" \
            "}\n" \
            "\n" \
            "key M {\n" \
            "    label:                              'M'\n" \
            "    base:                               'm'\n" \
            "    shift, capslock:                    'M'\n" \
            "}\n" \
            "\n" \
            "key N {\n" \
            "    label:                              'N'\n" \
            "    base:                               'n'\n" \
            "    shift, capslock:                    'N'\n" \
            "    alt:                                '\\u0303'\n" \
            "}\n" \
            "\n" \
            "key O {\n" \
            "    label:                              'O'\n" \
            "    base:                               'o'\n" \
            "    shift, capslock:                    'O'\n" \
            "}\n" \
            "\n" \
            "key P {\n" \
            "    label:                              'P'\n" \
            "    base:                               'p'\n" \
            "    shift, capslock:                    'P'\n" \
            "}\n" \
            "\n" \
            "key Q {\n" \
            "    label:                              'Q'\n" \
            "    base:                               'q'\n" \
            "    shift, capslock:                    'Q'\n" \
            "}\n" \
            "\n" \
            "key R {\n" \
            "    label:                              'R'\n" \
            "    base:                               'r'\n" \
            "    shift, capslock:                    'R'\n" \
            "}\n" \
            "\n" \
            "key S {\n" \
            "    label:                              'S'\n" \
            "    base:                               's'\n" \
            "    shift, capslock:                    'S'\n" \
            "    alt:                                '\\u00df'\n" \
            "}\n" \
            "\n" \
            "key T {\n" \
            "    label:                              'T'\n" \
            "    base:                               't'\n" \
            "    shift, capslock:                    'T'\n" \
            "}\n" \
            "\n" \
            "key U {\n" \
            "    label:                              'U'\n" \
            "    base:                               'u'\n" \
            "    shift, capslock:                    'U'\n" \
            "    alt:                                '\\u0308'\n" \
            "}\n" \
            "\n" \
            "key V {\n" \
            "    label:                              'V'\n" \
            "    base:                               'v'\n" \
            "    shift, capslock:                    'V'\n" \
            "}\n" \
            "\n" \
            "key W {\n" \
            "    label:                              'W'\n" \
            "    base:                               'w'\n" \
            "    shift, capslock:                    'W'\n" \
            "}\n" \
            "\n" \
            "key X {\n" \
            "    label:                              'X'\n" \
            "    base:                               'x'\n" \
            "    shift, capslock:                    'X'\n" \
            "}\n" \
            "\n" \
            "key Y {\n" \
            "    label:                              'Y'\n" \
            "    base:                               'y'\n" \
            "    shift, capslock:                    'Y'\n" \
            "}\n" \
            "\n" \
            "key Z {\n" \
            "    label:                              'Z'\n" \
            "    base:                               'z'\n" \
            "    shift, capslock:                    'Z'\n" \
            "}\n" \
            "\n" \
            "key 0 {\n" \
            "    label:                              '0'\n" \
            "    base:                               '0'\n" \
            "    shift:                              ')'\n" \
            "}\n" \
            "\n" \
            "key 1 {\n" \
            "    label:                              '1'\n" \
            "    base:                               '1'\n" \
            "    shift:                              '!'\n" \
            "}\n" \
            "\n" \
            "key 2 {\n" \
            "    label:                              '2'\n" \
            "    base:                               '2'\n" \
            "    shift:                              '@'\n" \
            "}\n" \
            "\n" \
            "key 3 {\n" \
            "    label:                              '3'\n" \
            "    base:                               '3'\n" \
            "    shift:                              '#'\n" \
            "}\n" \
            "\n" \
            "key 4 {\n" \
            "    label:                              '4'\n" \
            "    base:                               '4'\n" \
            "    shift:                              '$'\n" \
            "}\n" \
            "\n" \
            "key 5 {\n" \
            "    label:                              '5'\n" \
            "    base:                               '5'\n" \
            "    shift:                              '%'\n" \
            "}\n" \
            "\n" \
            "key 6 {\n" \
            "    label:                              '6'\n" \
            "    base:                               '6'\n" \
            "    shift:                              '^'\n" \
            "    alt+shift:                          '\\u0302'\n" \
            "}\n" \
            "\n" \
            "key 7 {\n" \
            "    label:                              '7'\n" \
            "    base:                               '7'\n" \
            "    shift:                              '&'\n" \
            "}\n" \
            "\n" \
            "key 8 {\n" \
            "    label:                              '8'\n" \
            "    base:                               '8'\n" \
            "    shift:                              '*'\n" \
            "}\n" \
            "\n" \
            "key 9 {\n" \
            "    label:                              '9'\n" \
            "    base:                               '9'\n" \
            "    shift:                              '('\n" \
            "}\n" \
            "\n" \
            "key SPACE {\n" \
            "    label:                              ' '\n" \
            "    base:                               ' '\n" \
            "    alt, meta:                          fallback SEARCH\n" \
            "    ctrl:                               fallback LANGUAGE_SWITCH\n" \
            "}\n" \
            "\n" \
            "key ENTER {\n" \
            "    label:                              '\\n'\n" \
            "    base:                               '\\n'\n" \
            "}\n" \
            "\n" \
            "key TAB {\n" \
            "    label:                              '\\t'\n" \
            "    base:                               '\\t'\n" \
            "}\n" \
            "\n" \
            "key COMMA {\n" \
            "    label:                              ','\n" \
            "    base:                               ','\n" \
            "    shift:                              '<'\n" \
            "}\n" \
            "\n" \
            "key PERIOD {\n" \
            "    label:                              '.'\n" \
            "    base:                               '.'\n" \
            "    shift:                              '>'\n" \
            "}\n" \
            "\n" \
            "key SLASH {\n" \
            "    label:                              '/'\n" \
            "    base:                               '/'\n" \
            "    shift:                              '?'\n" \
            "}\n" \
            "\n" \
            "key GRAVE {\n" \
            "    label:                              '`'\n" \
            "    base:                               '`'\n" \
            "    shift:                              '~'\n" \
            "    alt:                                '\\u0300'\n" \
            "    alt+shift:                          '\\u0303'\n" \
            "}\n" \
            "\n" \
            "key MINUS {\n" \
            "    label:                              '-'\n" \
            "    base:                               '-'\n" \
            "    shift:                              '_'\n" \
            "}\n" \
            "\n" \
            "key EQUALS {\n" \
            "    label:                              '='\n" \
            "    base:                               '='\n" \
            "    shift:                              '+'\n" \
            "}\n" \
            "\n" \
            "key LEFT_BRACKET {\n" \
            "    label:                              '['\n" \
            "    base:                               '['\n" \
            "    shift:                              '{'\n" \
            "}\n" \
            "\n" \
            "key RIGHT_BRACKET {\n" \
            "    label:                              ']'\n" \
            "    base:                               ']'\n" \
            "    shift:                              '}'\n" \
            "}\n" \
            "\n" \
            "key BACKSLASH {\n" \
            "    label:                              '\\\\'\n" \
            "    base:                               '\\\\'\n" \
            "    shift:                              '|'\n" \
            "}\n" \
            "\n" \
            "key SEMICOLON {\n" \
            "    label:                              ';'\n" \
            "    base:                               ';'\n" \
            "    shift:                              ':'\n" \
            "}\n" \
            "\n" \
            "key APOSTROPHE {\n" \
            "    label:                              '\\''\n" \
            "    base:                               '\\''\n" \
            "    shift:                              '\"'\n" \
            "}\n" \
            "\n" \
            "### Numeric keypad ###\n" \
            "\n" \
            "key NUMPAD_0 {\n" \
            "    label:                              '0'\n" \
            "    base:                               fallback INSERT\n" \
            "    numlock:                            '0'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_1 {\n" \
            "    label:                              '1'\n" \
            "    base:                               fallback MOVE_END\n" \
            "    numlock:                            '1'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_2 {\n" \
            "    label:                              '2'\n" \
            "    base:                               fallback DPAD_DOWN\n" \
            "    numlock:                            '2'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_3 {\n" \
            "    label:                              '3'\n" \
            "    base:                               fallback PAGE_DOWN\n" \
            "    numlock:                            '3'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_4 {\n" \
            "    label:                              '4'\n" \
            "    base:                               fallback DPAD_LEFT\n" \
            "    numlock:                            '4'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_5 {\n" \
            "    label:                              '5'\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "    numlock:                            '5'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_6 {\n" \
            "    label:                              '6'\n" \
            "    base:                               fallback DPAD_RIGHT\n" \
            "    numlock:                            '6'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_7 {\n" \
            "    label:                              '7'\n" \
            "    base:                               fallback MOVE_HOME\n" \
            "    numlock:                            '7'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_8 {\n" \
            "    label:                              '8'\n" \
            "    base:                               fallback DPAD_UP\n" \
            "    numlock:                            '8'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_9 {\n" \
            "    label:                              '9'\n" \
            "    base:                               fallback PAGE_UP\n" \
            "    numlock:                            '9'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_LEFT_PAREN {\n" \
            "    label:                              '('\n" \
            "    base:                               '('\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_RIGHT_PAREN {\n" \
            "    label:                              ')'\n" \
            "    base:                               ')'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_DIVIDE {\n" \
            "    label:                              '/'\n" \
            "    base:                               '/'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_MULTIPLY {\n" \
            "    label:                              '*'\n" \
            "    base:                               '*'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_SUBTRACT {\n" \
            "    label:                              '-'\n" \
            "    base:                               '-'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_ADD {\n" \
            "    label:                              '+'\n" \
            "    base:                               '+'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_DOT {\n" \
            "    label:                              '.'\n" \
            "    base:                               fallback FORWARD_DEL\n" \
            "    numlock:                            '.'\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_COMMA {\n" \
            "    label:                              ','\n" \
            "    base:                               ','\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_EQUALS {\n" \
            "    label:                              '='\n" \
            "    base:                               '='\n" \
            "}\n" \
            "\n" \
            "key NUMPAD_ENTER {\n" \
            "    label:                              '\\n'\n" \
            "    base:                               '\\n' fallback ENTER\n" \
            "    ctrl, alt, meta:                    none fallback ENTER\n" \
            "}\n" \
            "\n" \
            "### Special keys on phones ###\n" \
            "\n" \
            "key AT {\n" \
            "    label:                              '@'\n" \
            "    base:                               '@'\n" \
            "}\n" \
            "\n" \
            "key STAR {\n" \
            "    label:                              '*'\n" \
            "    base:                               '*'\n" \
            "}\n" \
            "\n" \
            "key POUND {\n" \
            "    label:                              '#'\n" \
            "    base:                               '#'\n" \
            "}\n" \
            "\n" \
            "key PLUS {\n" \
            "    label:                              '+'\n" \
            "    base:                               '+'\n" \
            "}\n" \
            "\n" \
            "### Non-printing keys ###\n" \
            "\n" \
            "key ESCAPE {\n" \
            "    base:                               fallback BACK\n" \
            "    alt, meta:                          fallback HOME\n" \
            "    ctrl:                               fallback MENU\n" \
            "}\n" \
            "\n" \
            "### Gamepad buttons ###\n" \
            "\n" \
            "key BUTTON_A {\n" \
            "    base:                               fallback BACK\n" \
            "}\n" \
            "\n" \
            "key BUTTON_B {\n" \
            "    base:                               fallback BACK\n" \
            "}\n" \
            "\n" \
            "key BUTTON_C {\n" \
            "    base:                               fallback BACK\n" \
            "}\n" \
            "\n" \
            "key BUTTON_X {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_Y {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_Z {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_L1 {\n" \
            "    base:                               none\n" \
            "}\n" \
            "\n" \
            "key BUTTON_R1 {\n" \
            "    base:                               none\n" \
            "}\n" \
            "\n" \
            "key BUTTON_L2 {\n" \
            "    base:                               none\n" \
            "}\n" \
            "\n" \
            "key BUTTON_R2 {\n" \
            "    base:                               none\n" \
            "}\n" \
            "\n" \
            "key BUTTON_THUMBL {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_THUMBR {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_START {\n" \
            "    base:                               fallback HOME\n" \
            "}\n" \
            "\n" \
            "key BUTTON_SELECT {\n" \
            "    base:                               fallback MENU\n" \
            "}\n" \
            "\n" \
            "key BUTTON_MODE {\n" \
            "    base:                               fallback MENU\n" \
            "}\n" \
            "\n" \
            "key BUTTON_1 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_2 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_3 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_4 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_5 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_6 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_7 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_8 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_9 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_10 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_11 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_12 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_13 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_14 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_15 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}\n" \
            "\n" \
            "key BUTTON_16 {\n" \
            "    base:                               fallback DPAD_CENTER\n" \
            "}";
    return result;
}
