# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 2 or 3,
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

struct Rectangle
{
    left @0 :Int32;
    top @1 :Int32;
    width @2 :UInt32;
    height @3 :UInt32;
}

struct KeyboardEvent
{
    action @0 :Action;
    keyCode @1 :Int32;
    scanCode @2 :Int32;

    enum Action
    {
        up @0;
        down @1;
        repeat @2;
    }
    text @3 :Text;
}

struct TouchScreenEvent
{
    struct Contact
    {
        id @0 :Int32;
        x @1 :Float32;
        y @2 :Float32;
        touchMajor @3 :Float32;
        touchMinor @4 :Float32;
        pressure @5 :Float32;
        orientation @6 :Float32;

        toolType @7 :ToolType;
        action @8 :TouchAction;

        enum TouchAction
        {
            up @0;
            down @1;
            change @2;
        }
        enum ToolType
        {
            unknown @0;
            finger @1;
            stylus @2;
        }
    }

    buttons @0 :UInt32;

    count @1 :UInt32;
    contacts @2 :List(Contact);
    const maxCount :UInt32 = 16;
}

struct PointerEvent
{
    x @0 :Float32;
    y @1 :Float32;
    dx @2 :Float32;
    dy @3 :Float32;
    vscroll @4 :Float32;
    hscroll @5 :Float32;

    action @6 :PointerAction;

    buttons @7 :UInt32;

    dndHandle @8 :List(UInt8);

    enum PointerAction
    {
       up @0;
       down @1;
       enter @2;
       leave @3;
       motion @4;
    }
}

struct InputEvent
{
    deviceId @0 :InputDeviceId;
    cookie @1 :Data;
    eventTime @2 :NanoSeconds;
    modifiers @3 :UInt32;

    union
    {
       key @4 : KeyboardEvent;
       touch @5 : TouchScreenEvent;
       pointer @6 : PointerEvent;
    }

    windowId @7 :Int32;
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
    dndHandle @3 :List(UInt8);

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
        startDragAndDrop @7;
        # Must be last
        surfaceAttrib @8;
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
    refreshRate @5 :Float64;

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

struct InputDeviceStateEvent
{
    when @0 :NanoSeconds;
    buttons @1 :UInt32;
    modifiers @2 :UInt32;
    pointerX @3 :Float32;
    pointerY @4 :Float32;
    devices @5 :List(DeviceState);

    struct DeviceState
    {
        deviceId @0 :InputDeviceId;
        pressedKeys @1 :List(UInt32);
        buttons @2 :UInt32;
    }

    windowId @6 :Int32;
}

struct SurfacePlacementEvent
{
    id @0 :Int32;
    rectangle @1 :Rectangle;
}

struct Event
{
    union
    {
        input @0 :InputEvent;
        surface @1 :SurfaceEvent;
        resize @2 :ResizeEvent;
        promptSession @3 :PromptSessionEvent;
        orientation @4 :OrientationEvent;
        closeSurface @5 :CloseSurfaceEvent;
        keymap @6 :KeymapEvent;
        inputConfiguration @7 :InputConfigurationEvent;
        surfaceOutput @8 :SurfaceOutputEvent;
        inputDevice @9 :InputDeviceStateEvent;
        surfacePlacement @10 :SurfacePlacementEvent;
    }
}
