# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Author: Andreas Pokorny <andreas.pokorny@canonical.com>

@0xbc7cebb39c9c4f2f;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("mir::capnp");

using MirEvent = import "mir_event.capnp";

struct Keymap
{
    model @0 :Text;
    layout @1 :Text;
    variant @2 :Text;
    options @3 :Text;
}

struct PointerConfiguration
{
    enum Handedness
    {
        right @0;
        left @1;
    }
    enum Acceleration
    {
        none @0;
        adaptive @1;
    }
    handedness @0 :Handedness;
    acceleration @1 :Acceleration;
    cursorAccelerationBias @2 :Float32;
    horizontalScrollScale @3 :Float32;
    verticalScrollScale @4 :Float32;
}

struct TouchscreenConfiguration 
{
    enum MappingMode
    {
       toOutput @0;
       toDisplayWall @1;
    }
    outputId @0 :Float32;
    mappingMode @1 :MappingMode;
}

struct TouchpadConfiguration
{
    clickMode @0 :UInt32;
    scrollMode @1 :UInt32;
    buttonDownScrollButton @2 :Int32;
    tapToClick @3 :UInt32;
    middleMouseButtonEmulation @4 :UInt32;
    disableWithMouse @5 :UInt32;
    disableWhileTyping @6 :UInt32;
}

struct KeyboardConfiguration
{
   keymap @0 :Keymap;
}

struct DeviceConfiguration
{
   id @0 :MirEvent.InputDeviceId; 
   capabilities @1 :Int32;
   name @2 :Text;
   uniqueId @3 :Text;

   pointerConfiguration @4 :PointerConfiguration;
   touchpadConfiguration @5 :TouchpadConfiguration;
   keyboardConfiguration @6 :KeyboardConfiguration;
   touchscreenConfiguration @7 :TouchscreenConfiguration;
}

struct InputConfiguration
{
    devices @0 :List(DeviceConfiguration);
}
