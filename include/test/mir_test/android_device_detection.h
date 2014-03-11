/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_ANDROID_DEVICE_DETECTION_H_
#define MIR_TEST_ANDROID_DEVICE_DETECTION_H_

#include "src/platform/graphics/android/device_detector.h"
#include <gtest/gtest.h>
#include <iostream>

#define SKIP_IF_NO_ANDROID_HARDWARE_PRESENT() \
    mir::graphics::android::PropertiesOps ops; \
    mir::graphics::android::DeviceDetector detector{ops}; \
    if(!detector.android_device_present()) \
    { \
        std::cout << "WARNING: test skipped due to lack of android drivers on system\n"; \
        SUCCEED(); \
        return; \
    }

#endif /* MIR_TEST_ANDROID_DEVICE_DETECTION_H_ */
