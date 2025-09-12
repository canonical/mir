(mir-input-platform)=

# Input Platform APIs

## Introduction

The Mir input platform is provided by a loadable module implementing the
following entrypoints using C linkage (i.e. no symbol name mangling):

```c++
mir::ModuleProperties const* describe_input_module();
```

Describes the module: the name, version, and shared object file that
implements it.

```c++
void add_input_platform_options(boost::program_options::options_description& config);
```

Adds command line options specific to this platform to the provided
options description object.

This is not currently called, as detailed in
[mir #4109](https://github.com/canonical/mir/issues/4109).

```c++
mir::input::PlatformPriority probe_input_platform(
    mir::options::Option const& options,
    mir::ConsoleServices& console);
```

Report how suitable this input platform module for to the runtime
environment. The return value can range from saying the module is
unsupported all the way to being the best choice for the system.

```c++
mir::UniqueModulePtr<mir::input::Platform> create_input_platform(
    mir::options::Option const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::input::InputReport> const& report);
```

Instantiate the input platform implementation. This factory function is provided with the objects it will need to interact with the rest of the server:

- `options` provides access to command line options.
- `emergency_cleanup_registry` provides a way to register handlers that should run on abnormal terminations of the server.
- `input_device_registry` is a place to register discovered input devices.
- `console` provides APIs for interacting with console. This includes VT switching, and requesting access to hardware via logind.
- `report` provides an API to log input events. This is not the primary way input is handled in the server: it is instead used for debug logging and tracing.

In turn, the function returns a {class}`mir::input::Platform` subclass that
the server uses to control the input platform implementation.

## The mir::input::Platform type

The returned {class}`mir::input::Platform` instance is responsible for
monitoring for available input devices: both pre-existing and newly
hotplugged.

The `start`, `stop`, `pause_for_config`, and `continue_after_config`
methods allow the server to control when the input platform should
monitor for devices.

When new devices are plugged into the system, they are registered with
the {class}`mir::input::InputDeviceRegistry` via it's `add_device`
method. When they are unplugged, it should call the `remove_device`
method.

The platform has a `dispatchable` method that allows integration into
the server's IO event loop.

## The mir::input::InputDevice type

Input devices added to the registry are represented by
{class}`mir::input::InputDevice` subclasses. The subclass must implement the
following methods:

- `start` and `stop` to start/stop consuming events from the device.
- `get_device_info` to retrieve an {class}`mir::input::InputDeviceInfo` struct describing the device.
- `get_pointer_settings`, `get_touchpad_settings`, `get_touchscreen_settings`, and `apply_settings` to manage settings of the device.

If reading events from the input device requires the use of the server
event loop, this should be achieved through the platform's
`dispatchable` method.

### Processing events

When the server calls the `start` method on an {class}`InputDevice <mir::input::InputDevice>`, it will pass an {class}`InputSink <mir::input::InputSink>` object and an {class}`EventBuilder <mir::input::EventBuilder>` object.

Each time the device generates an event, the {class}`InputDevice <mir::input::InputDevice>` is responsible for calling the sink's
`handle_event` method. This method takes a {struct}`MirEvent` pointer
as an argument. Rather than constructing the event objects itself, the
provided `EventBuilder` class should be used instead.

The `EventBuilder` class includes methods for constructing various
types of input events (keyboard events, mouse pointer events,
touchscreen events, etc). This gives the consumer a way to customise
how the events are represented.

In addition to sending events to the `InputSink`, the device is also
expected to send a more raw version of the event data to the
platform's {class}`InputReport <mir::input::InputReport>` object to
enable tracing.
