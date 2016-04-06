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
# Author: Brandon Schaefer <brandon.schaefer@canonical.com>

@0x9ff9e89b8e11dc56;

# Need to namespace the InputEvent/Event since the generated (Mir)Event has all its ctors deleted.
# Which we need a C compliant struct to be generated. So namespace these, and wrap a
# builder in a struct MirEvent {..};
using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("mir::capnp");

struct NanoSeconds
{
    count @0 :Int64;
}

struct InputDeviceId
{
    id @0 :UInt64;
}

struct KeyboardEvent
{
    id @0 :Int32;
    deviceId @1 :InputDeviceId;
    sourceId @2 :Int32;
    action @3 :Action;
    modifiers @4 :UInt32;
    keyCode @5 :Int32;
    scanCode @6 :Int32;
    eventTime @7 :NanoSeconds;
    cookie @8 :Data;

    enum Action
    {
        up @0;
        down @1;
        repeat @2;
    }
}

struct MotionSetEvent
{
    struct Motion
    {
        id @0 :Int32;
        x @1 :Float32;
        y @2 :Float32;
        dx @3 :Float32;
        dy @4 :Float32;
        touchMajor @5 :Float32;
        touchMinor @6 :Float32;
        size @7 :Float32;
        pressure @8 :Float32;
        orientation @9 :Float32;
        vscroll @10 :Float32;
        hscroll @11 :Float32;

        toolType @12 :ToolType;

        # TODO: We would like to store this as a TouchAction but we still encode pointer actions
        # here as well.
        action @13 :Int32;

        enum ToolType
        {
            unknown @0;
            finger @1;
            stylus @2;
        }
    }

    deviceId @0 :InputDeviceId;
    sourceId @1 :Int32;
    modifiers @2 :UInt32;
    buttons @3 :UInt32;
    eventTime @4 :NanoSeconds;
    cookie @5 :Data;

    count @6 :UInt32;
    motions @7 :List(Motion);
    const maxCount :UInt32 = 16;
}

struct InputConfigurationEvent
{
    action @0 :Action;
    when @1 :NanoSeconds;
    id @2 :InputDeviceId;

    enum Action 
    {
        configurationChanged @0;
        deviceReset @1;
    }
}

struct SurfaceEvent
{
    id @0 :Int32;
    attrib @1 :Attrib;
    value @2 :Int32;

    enum Attrib
    {
        # Do not specify values...code relies on 0...N ordering.
        type @0;
        state @1;
        swapInterval @2;
        focus @3;
        dpi @4;
        visibility @5;
        preferredOrientation @6;
        # Must be last
        surfaceAttrib @7;
    }
}

struct ResizeEvent
{
    surfaceId @0 :Int32;
    width @1 :Int32;
    height @2 :Int32;
}

struct PromptSessionEvent
{
    newState @0 :State;

    enum State
    {
        stopped @0;
        started @1;
        suspended @2;
    }
}

struct OrientationEvent
{
    surfaceId @0 :Int32;
    direction @1 :Int32;
}

struct CloseSurfaceEvent
{
    surfaceId @0 :Int32;
}

struct KeymapEvent
{
    surfaceId @0 :Int32;

    deviceId @1 :InputDeviceId;
    buffer @2 :Text;
}

struct SurfaceOutputEvent
{
    surfaceId @0 :Int32;
    dpi @1 :Int32;
    scale @2 :Float32;
    formFactor @3 :FormFactor;
    outputId @4 :UInt32;

    enum FormFactor
    {
        unknown @0;
        phone @1;
        tablet @2;
        monitor @3;
        tv @4;
        projector @5;
    }
}

struct Event
{
    union
    {
        key @0 :KeyboardEvent;
        motionSet @1 :MotionSetEvent;
        surface @2 :SurfaceEvent;
        resize @3 :ResizeEvent;
        promptSession @4 :PromptSessionEvent;
        orientation @5 :OrientationEvent;
        closeSurface @6 :CloseSurfaceEvent;
        keymap @7 :KeymapEvent;
        inputConfiguration @8 :InputConfigurationEvent;
        surfaceOutput @9 :SurfaceOutputEvent;
    }
}
