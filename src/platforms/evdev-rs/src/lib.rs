use cxx::{SharedPtr, UniquePtr, WeakPtr};
use input::event::{DeviceEvent, EventTrait};
use input::{AsRaw, Device, Event, Libinput, LibinputInterface};
use libc::{major, minor, poll, pollfd, stat, POLLIN};
use nix::unistd::close;
use std::ffi::CString;
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, FromRawFd};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::io::OwnedFd;
use std::path::Path;
use std::ptr::NonNull;
use std::sync::mpsc;
use std::thread::{self, JoinHandle};

mod enums;

struct LibinputInterfaceImpl {
    bridge: SharedPtr<PlatformBridgeC>,
    fds: Vec<UniquePtr<DeviceBridgeC>>,
}

impl LibinputInterface for LibinputInterfaceImpl {
    // This method is called whenever libinput needs to open a new device.
    fn open_restricted(&mut self, path: &Path, flags: i32) -> Result<OwnedFd, i32> {
        println!("Opening device: {:?} with flags: {}", path, flags);
        let cpath = CString::new(path.as_os_str().as_bytes()).unwrap();

        let mut st: stat = unsafe { std::mem::zeroed() };
        let ret = unsafe { libc::stat(cpath.as_ptr(), &mut st) };
        if ret != 0 {
            return Err(ret);
        }

        let major_num = major(st.st_rdev);
        let minor_num = minor(st.st_rdev);

        // By keeping the device referenced in the fds vector, we ensure it stays alive
        // until close_restricted is called.
        let device = self
            .bridge
            .acquire_device(major_num as i32, minor_num as i32);
        let fd = device.raw_fd();
        println!(
            "Acquired fd: {} for device with major: {}, minor: {}",
            fd, major_num, minor_num
        );
        self.fds.push(device);

        let owned = unsafe { OwnedFd::from_raw_fd(fd) };
        println!("Owned fd: {}", owned.as_raw_fd().to_string());
        Ok(owned)
    }

    // This method is called when libinput is done with a device.
    fn close_restricted(&mut self, fd: OwnedFd) {
        let raw_fd = fd.as_raw_fd();
        println!("Closing device with fd: {}", raw_fd.to_string());
        let _ = close(fd);
        // self.fds.retain(|f| f.as_raw_fd() != raw_fd);
    }
}

// This is a bit of a hack here.
//
// The other side of our platform bridge is known by us to be thread-safe, so we can
// safely send it to another thread. However, Rust has no way of knowing that, so
// we have to assert it ourselves.
unsafe impl Send for PlatformBridgeC {}
unsafe impl Sync for PlatformBridgeC {}
unsafe impl Send for InputDeviceRegistry {}
unsafe impl Sync for InputDeviceRegistry {}
unsafe impl Send for InputDevice {}
unsafe impl Sync for InputDevice {}
unsafe impl Send for InputSink {}
unsafe impl Sync for InputSink {}
unsafe impl Send for EventBuilder {}
unsafe impl Sync for EventBuilder {}

// Rust :(
//
// Because *mut InputSink and *mut EventBuilder are raw pointers, Rust assumes
// that they are neither Send nor Sync. However, we know that the other side of
// the pointer is actually a C++ object that is thread-safe, so we can assert
// that these pointers are Send and Sync. We cannot define Send and Sync on the
// raw types to fix the issue unfortuately, so we have to wrap them in a newtype
// and define Send and Sync on that.
struct InputSinkPtr(NonNull<InputSink>);
unsafe impl Send for InputSinkPtr {}
unsafe impl Sync for InputSinkPtr {}

struct EventBuilderPtr(NonNull<EventBuilder>);
unsafe impl Send for EventBuilderPtr {}
unsafe impl Sync for EventBuilderPtr {}

struct DeviceWrapper {
    id: i32,
    device: Device,
    input_device: SharedPtr<InputDevice>,
    input_sink: Option<InputSinkPtr>,
    event_builder: Option<UniquePtr<EventBuilderWrapper>>,
}

struct LibinputState {
    libinput: Libinput,
    known_devices: Vec<DeviceWrapper>,
    next_device_id: i32,
}

pub struct PlatformRs {
    bridge: SharedPtr<PlatformBridgeC>,
    device_registry: SharedPtr<InputDeviceRegistry>,
    handle: Option<JoinHandle<()>>,
    wfd: Option<OwnedFd>,
    tx: Option<mpsc::Sender<ThreadCommand>>,
}

pub struct InputDeviceInfoRs {
    name: String,
    unique_id: String,
    capabilities: i32,
}

impl InputDeviceInfoRs {
    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn unique_id(&self) -> &str {
        &self.unique_id
    }

    pub fn capabilities(&self) -> i32 {
        self.capabilities
    }
}

pub enum ThreadCommand {
    Start(i32, InputSinkPtr, EventBuilderPtr),
    GetDeviceInfo(i32, mpsc::Sender<InputDeviceInfoRs>),
}

// This is so silly.
//
// We can't create the channel inside of the PlatformRs implementation because
// the "mod ffi" block gets confused when parsing the template, so it thinks that the
// closing bracket is an equality operator. Moving it out to the global scope fixes
// things, but this is so silly.
fn create_thread_command_channel() -> (mpsc::Sender<ThreadCommand>, mpsc::Receiver<ThreadCommand>) {
    mpsc::channel()
}

fn create_device_info_rs_channel() -> (
    mpsc::Sender<InputDeviceInfoRs>,
    mpsc::Receiver<InputDeviceInfoRs>,
) {
    mpsc::channel()
}

impl PlatformRs {
    pub fn start(&mut self) {
        println!("Starting evdev-rs platform");

        let bridge = self.bridge.clone();
        let device_registry = self.device_registry.clone();
        let (rfd, wfd) = nix::unistd::pipe().unwrap();
        let (tx, rx) = create_thread_command_channel();

        self.wfd = Some(wfd);
        self.tx = Some(tx);
        self.handle = Some(thread::spawn(move || {
            PlatformRs::run(bridge, device_registry, rfd, rx);
        }));
    }

    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}
    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn create_input_device(&self, device_id: i32) -> Box<InputDeviceRs> {
        Box::new(InputDeviceRs {
            device_id: device_id,
            wfd: self.wfd.as_ref().unwrap().as_fd(),
            tx: self.tx.clone().unwrap(),
        })
    }

    fn run(
        bridge_locked: SharedPtr<PlatformBridgeC>,
        device_registry: SharedPtr<InputDeviceRegistry>,
        rfd: OwnedFd,
        rx: std::sync::mpsc::Receiver<ThreadCommand>,
    ) {
        let mut state = LibinputState {
            libinput: Libinput::new_with_udev(LibinputInterfaceImpl {
                bridge: bridge_locked.clone(),
                fds: Vec::new(),
            }),
            known_devices: Vec::new(),
            next_device_id: 0,
        };

        state.libinput.udev_assign_seat("seat0").unwrap();

        let mut fds = [
            pollfd {
                fd: state.libinput.as_raw_fd(),
                events: POLLIN,
                revents: 0,
            },
            pollfd {
                fd: rfd.as_raw_fd(),
                events: POLLIN,
                revents: 0,
            },
        ];

        loop {
            println!("Waiting for input events...");
            unsafe {
                let ret = poll(fds.as_mut_ptr(), fds.len() as _, -1); // -1 = wait indefinitely
                if ret < 0 {
                    println!("Error polling libinput fd");
                    return;
                }
            }

            if (fds[0].revents & POLLIN) != 0 {
                if state.libinput.dispatch().is_err() {
                    // TODO: Report the error to Mir's logging facilities somehow
                    println!("Error dispatching libinput events");
                    return;
                }

                println!("Processing input events");
                for event in &mut state.libinput {
                    println!("Got event: {:?}", event);

                    match event {
                        Event::Device(device_event) => match device_event {
                            DeviceEvent::Added(added_event) => {
                                let dev: Device = added_event.device();
                                state.known_devices.push(DeviceWrapper {
                                    id: state.next_device_id,
                                    device: dev,
                                    input_device: bridge_locked
                                        .create_input_device(state.next_device_id),
                                    input_sink: None,
                                    event_builder: None,
                                });

                                // The device registry may call back into the input device, but the input device
                                // may call back into this thread to retrieve information. To avoid deadlocks, we
                                // have to queue this work onto a new thread to fire and forget it. It's not a big
                                // deal, but it is a bit unfortunate.
                                let input_device =
                                    state.known_devices.last().unwrap().input_device.clone();
                                let device_registry = device_registry.clone();
                                thread::spawn(move || unsafe {
                                    device_registry
                                        .clone()
                                        .pin_mut_unchecked()
                                        .add_device(&input_device);
                                });

                                state.next_device_id += 1;
                            }
                            DeviceEvent::Removed(removed_event) => {
                                let dev: Device = removed_event.device();
                                let index = state
                                    .known_devices
                                    .iter()
                                    .position(|x| x.device.as_raw() == dev.as_raw());
                                if let Some(index) = index {
                                    // unsafe {
                                    //     device_registry.clone().pin_mut_unchecked().remove_device(&state.known_devices[index].input_device);
                                    // }
                                    state.known_devices.remove(index);
                                }
                            }
                            _ => {}
                        },

                        Event::Pointer(pointer_event) => {
                            let dev: Device = pointer_event.device();

                            if let Some(device_wrapper) = state
                                .known_devices
                                .iter()
                                .find(|x| x.device.as_raw() == dev.as_raw())
                            {
                                println!("Pointer event: {:?}", pointer_event);
                            }
                        }

                        // TODO: Otherwise, find the event, process it, and request that the input sink handle it
                        _ => {}
                    }
                }
            }

            if (fds[1].revents & POLLIN) != 0 {
                let mut buf = [0u8];
                nix::unistd::read(&rfd, &mut buf).unwrap();
                // handle commands
                while let Ok(cmd) = rx.try_recv() {
                    match cmd {
                        ThreadCommand::GetDeviceInfo(id, tx) => {
                            // For simplicity, just return info about the first device
                            if let Some(dev_with_id) =
                                state.known_devices.iter().find(|d| d.id == id)
                            {
                                let info = InputDeviceInfoRs {
                                    name: dev_with_id.device.name().to_string(),
                                    unique_id: "TODO".to_string(),
                                    capabilities: 0,
                                };
                                let _ = tx.send(info);
                            } else {
                                let info = InputDeviceInfoRs {
                                    name: "".to_string(),
                                    unique_id: "".to_string(),
                                    capabilities: 0,
                                };
                                let _ = tx.send(info);
                            }
                        }
                        ThreadCommand::Start(id, input_sink, event_builder) => {
                            if let Some(dev_with_id) =
                                state.known_devices.iter_mut().find(|d| d.id == id)
                            {
                                dev_with_id.input_sink = Some(input_sink);
                                unsafe {
                                    dev_with_id.event_builder = Some(bridge_locked
                                        .create_event_builder_wrapper(event_builder.0.as_ptr()));
                                }
                                println!("Starting input device with id: {}", id);
                            }
                        }
                    }
                }
            }
        }
    }
}

pub struct DeviceObserverRs {
    fd: Option<i32>,
}

impl DeviceObserverRs {
    pub fn new() -> Self {
        DeviceObserverRs { fd: None }
    }

    pub fn activated(&mut self, fd: i32) {
        println!("Device activated with fd: {}", fd);
        self.fd = Some(fd);
    }

    pub fn suspended(&mut self) {
        println!("Device suspended");
    }

    pub fn removed(&mut self) {
        println!("Device removed");
        self.fd = None;
    }
}

pub struct InputDeviceRs<'fd> {
    device_id: i32,
    wfd: BorrowedFd<'fd>,
    tx: mpsc::Sender<ThreadCommand>,
}

impl<'fd> InputDeviceRs<'fd> {
    pub fn start(&mut self, input_sink: *mut InputSink, event_builder: *mut EventBuilder) {
        self.tx
            .send(ThreadCommand::Start(
                self.device_id,
                InputSinkPtr(NonNull::new(input_sink).unwrap()),
                EventBuilderPtr(NonNull::new(event_builder).unwrap()),
            ))
            .unwrap();
        nix::unistd::write(&self.wfd, &[0u8]).unwrap();
    }

    pub fn stop(&mut self) {}

    pub fn get_device_info(&self) -> Box<InputDeviceInfoRs> {
        let (tx, rx) = create_device_info_rs_channel();
        self.tx
            .send(ThreadCommand::GetDeviceInfo(self.device_id, tx))
            .unwrap();
        nix::unistd::write(&self.wfd, &[0u8]).unwrap();
        let device_info = rx.recv().unwrap();
        Box::new(device_info)
    }
}

#[cxx::bridge(namespace = "mir::input::evdev_rs")]
mod ffi {
    extern "Rust" {
        type PlatformRs;
        type DeviceObserverRs;
        type InputDeviceRs<'fd>;
        type InputDeviceInfoRs;

        fn start(self: &mut PlatformRs);
        fn continue_after_config(self: &PlatformRs);
        fn pause_for_config(self: &PlatformRs);
        fn stop(self: &PlatformRs);
        fn create_device_observer(self: &PlatformRs) -> Box<DeviceObserverRs>;
        fn create_input_device(self: &PlatformRs, device_id: i32) -> Box<InputDeviceRs>;

        fn activated(self: &mut DeviceObserverRs, fd: i32);
        fn suspended(self: &mut DeviceObserverRs);
        fn removed(self: &mut DeviceObserverRs);

        unsafe fn start(
            self: &mut InputDeviceRs,
            input_sink: *mut InputSink,
            event_builder: *mut EventBuilder,
        );
        fn stop(self: &mut InputDeviceRs);
        fn get_device_info(self: &InputDeviceRs) -> Box<InputDeviceInfoRs>;

        fn name(self: &InputDeviceInfoRs) -> &str;
        fn unique_id(self: &InputDeviceInfoRs) -> &str;
        fn capabilities(self: &InputDeviceInfoRs) -> i32;

        fn evdev_rs_create(
            bridge: SharedPtr<PlatformBridgeC>,
            device_registry: SharedPtr<InputDeviceRegistry>,
        ) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("platform_bridge.h");
        include!("mir/input/input_device_registry.h");
        include!("mir/input/device.h");
        include!("mir/input/input_sink.h");
        include!("mir/input/event_builder.h");
        include!("mir/events/event.h");
        include!("mir/events/scroll_axis.h");
        include!("mir_toolkit/events/enums.h");
        include!("mir/geometry/point.h");
        include!("mir/geometry/displacement.h");
        include!("mir/geometry/forward.h");

        type PlatformBridgeC;
        type DeviceBridgeC;
        type EventBuilderWrapper;
        type EventUPtrWrapper;

        #[namespace = "mir::input"]
        type Device;

        #[namespace = "mir::input"]
        type InputDevice;

        #[namespace = "mir::input"]
        type InputDeviceRegistry;

        #[namespace = "mir::input"]
        type InputSink;

        #[namespace = "mir::input"]
        type EventBuilder;

        #[namespace = ""]
        type MirEvent;

        #[namespace = "mir::geometry"]
        type PointF;

        #[namespace = "mir::geometry"]
        type DisplacementF;

        fn acquire_device(
            self: &PlatformBridgeC,
            major: i32,
            minor: i32,
        ) -> UniquePtr<DeviceBridgeC>;
        fn create_input_device(self: &PlatformBridgeC, device_id: i32) -> SharedPtr<InputDevice>;
        fn raw_fd(self: &DeviceBridgeC) -> i32;
        unsafe fn create_event_builder_wrapper(self: &PlatformBridgeC, event_builder: *mut EventBuilder) -> UniquePtr<EventBuilderWrapper>;

        // CXX-Rust doesn't support passing Option<T> to C++ functions, so I use booleans
        // instead.
        //
        // On top of this, all enums are changed to i32 for ABI stability.
        fn pointer_event(
            self: &EventBuilderWrapper,
            has_time: bool,
            time_nanoseconds: u64,
            action: i32,
            buttons: u32,
            has_position: bool, 
            position_x: f32,
            position_y: f32,
            displacement_x: f32,
            displacement_y: f32,
            axis_source: i32
        ) -> UniquePtr<EventUPtrWrapper>;

        #[namespace = "mir::input"]
        fn add_device(
            self: Pin<&mut InputDeviceRegistry>,
            device: &SharedPtr<InputDevice>,
        ) -> WeakPtr<Device>;

        #[namespace = "mir::input"]
        fn remove_device(self: Pin<&mut InputDeviceRegistry>, device: &SharedPtr<InputDevice>);

        #[namespace = "mir::input"]
        fn handle_input(self: Pin<&mut InputSink>, event: &SharedPtr<MirEvent>);
    }
}

pub use ffi::DeviceBridgeC;
pub use ffi::EventBuilder;
pub use ffi::EventBuilderWrapper;
pub use ffi::InputDevice;
pub use ffi::InputDeviceRegistry;
pub use ffi::InputSink;
pub use ffi::PlatformBridgeC;

pub fn evdev_rs_create(
    bridge: SharedPtr<PlatformBridgeC>,
    device_registry: SharedPtr<InputDeviceRegistry>,
) -> Box<PlatformRs> {
    return Box::new(PlatformRs {
        bridge: bridge,
        device_registry: device_registry,
        handle: None,
        wfd: None,
        tx: None,
    });
}
