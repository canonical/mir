(mir-input-platform)=

# Input Platform APIs
This document explains the input platform API. Developers may implement this
API in order to define their own input platform. The Mir project ships with
`evdev`, `X11` and `Wayland` input platforms that developers may use as references.

## Required Entrypoints
The Mir input platform is provided by a loadable module. Developers must
implement the following entrypoints using C linkage (i.e. no symbol name mangling):

- `describe_input_module`:

    ```c++
    mir::ModuleProperties const* describe_input_module();
    ```

    Describes the module: the name, version, and shared object file that
    implements it.

- `add_input_platform_options` (optional):

    ```c++
    void add_input_platform_options(boost::program_options::options_description& config);
    ```

    Adds command line options specific to this platform to the provided
    options description object.

    This is not currently called, as detailed in
    [mir #4109](https://github.com/canonical/mir/issues/4109).

- `probe_input_platform`:

    ```c++
    mir::input::PlatformPriority probe_input_platform(
        mir::options::Option const& options,
        mir::ConsoleServices& console);
    ```

    Report how suitable this input platform module for to the runtime
    environment. The return value can range from saying the module is
    unsupported all the way to being the best choice for the system.

- `create_input_platform`:

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

## `mir::input::Platform`

`create_input_platform` will create an instance of {class}`mir::input::Platform`.
This class is responsible for monitoring the available input devices (both pre-existing and newly
hotplugged) in addition to listening for input events and forwarding them to the input subsystem in Mir. 

The `start`, `stop`, `pause_for_config`, and `continue_after_config`
methods allow the server to control when the input platform should
monitor for devices and input events. The platform may choose to respond to
these events however they see fit.

When new devices are plugged into the system, they are registered with
the {class}`mir::input::InputDeviceRegistry` via its `add_device`
method. When a device is unplugged, the platform should call the `remove_device`
method.

The platform implementation should define the `dispatchable` method. This method
returns a `mir::dispatch::Dispatchable`, which monitor the file descriptor
that is written to when the platform receives an input event. For example, the `evdev`
input platform will provide the `libinput` file descriptor in its dispatchable.

**Important note on thread safety**: Input events (adding/removing devices and key/pointer/touch/etc. events)
must _strictly_ be triggered by this dispatchable, as Mir's input subsystem assumes that all input actions
happen on the input thread. Developers must _not_ add/remove devices or create input events from
any other thread.

## `mir::input::InputDevice`
Input devices added to the registry are represented by
{class}`mir::input::InputDevice` subclasses. The subclass must implement the
following methods:

- `start` and `stop` to start/stop consuming events from the device.
- `get_device_info` to retrieve an {class}`mir::input::InputDeviceInfo` struct describing the device.
- `get_pointer_settings`, `get_touchpad_settings`, `get_touchscreen_settings`, and `apply_settings` to manage settings of the device.

Events originating from a particular input device _must_ be processed by the `mir::input::Platform::dispatchable`
so that they are created on the input thread. Do **not** create events for a device on any other thread.

### Event Processing
When the server calls the `start` method on an {class}`InputDevice <mir::input::InputDevice>`,
it will pass an {class}`InputSink <mir::input::InputSink>` object and an
{class}`EventBuilder <mir::input::EventBuilder>` object.

Each time the device generates an event, the {class}`InputDevice <mir::input::InputDevice>`
is responsible for calling the sink's `handle_event` method. This method takes a {struct}`MirEvent` pointer
as an argument. The {class}`mir::input::EventBuilder` class should be used to construct events.

The {class}`mir::input::EventBuilder` class includes methods for constructing various
types of input events (keyboard events, mouse pointer events,
touchscreen events, etc). This gives the consumer a way to customise
how the events are represented.
 
In addition to sending events to the `InputSink`, the device is also
expected to send a more raw version of the event data to the
platform's {class}`InputReport <mir::input::InputReport>` object to
enable tracing.
