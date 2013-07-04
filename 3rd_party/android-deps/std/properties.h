/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_PROPERTIES_H_
#define MIR_ANDROID_UBUNTU_PROPERTIES_H_

#include <cstring>

#define PROPERTY_VALUE_MAX  92

namespace mir_input
{
inline int property_get(const char *key, char *value, const char *default_value)
{
    int len = -1;
    if (default_value != NULL) {
        strcpy(value, default_value);
        len = strlen(value);
    } else {
        /*
         * If the value isn't defined, hand back an empty string and
         * a zero length, rather than a failure.  This seems wrong,
         * since you can't tell the difference between "undefined" and
         * "defined but empty", but it's what the device does.
         */
        value[0] = '\0';
        len = 0;
    }
    return len;
}
}

namespace android
{
using ::mir_input::property_get;
}

#endif /* MIR_ANDROID_UBUNTU_PROPERTIES_H_ */
