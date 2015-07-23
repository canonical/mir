#!/bin/sh
#
# Copyright © 2015 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
#

if [ -z "$1" ] ; then
    percent=100
else
    percent=$1
fi

for devdir in /sys/class/backlight/* /sys/class/leds/lcd-backlight ; do
    if [ -d "$devdir" ] ; then
        max=`cat $devdir/max_brightness`
        val=$(($max * $percent / 100))
        echo $val > $devdir/brightness
        echo "Set backlight $devdir to $percent% ($val/$max)"
    fi
done
