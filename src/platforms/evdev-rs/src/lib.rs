use input::event::{DeviceEvent, EventTrait};
use input::{AsRaw, Device, Event, Libinput, LibinputInterface};
use std::clone;
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, FromRawFd};
use std::path::Path;
use libc::{stat, major, minor, poll, pollfd, POLLIN};
use std::ffi::CString;
use std::os::unix::ffi::OsStrExt;
use std::os::unix::{io::OwnedFd};
use cxx::SharedPtr;
use cxx::UniquePtr;
use std::thread::{self, JoinHandle};
use nix::unistd::close;
use std::sync::mpsc;

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
        let device = self.bridge.acquire_device(major_num as i32, minor_num as i32);
        let fd = device.raw_fd();
        println!("Acquired fd: {} for device with major: {}, minor: {}", fd, major_num, minor_num);
        self.fds.push(device);

        let owned = unsafe { OwnedFd::from_raw_fd(fd)};
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

struct DeviceWithId {
    id: i32,
    device: Device
}


struct LibinputState {
    libinput: Libinput,
    known_devices: Vec<DeviceWithId>,
    next_device_id: i32,
}

pub struct PlatformRs {
    bridge: SharedPtr<PlatformBridgeC>,
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
    GetDeviceInfo(i32, mpsc::Sender<InputDeviceInfoRs>)
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

fn create_device_info_rs_channel() -> (mpsc::Sender<InputDeviceInfoRs>, mpsc::Receiver<InputDeviceInfoRs>) {
    mpsc::channel()
}

impl PlatformRs {
    pub fn start(&mut self) {
        println!("Starting evdev-rs platform");

        let bridge = self.bridge.clone(); // Arc clone, cheap, Send
        let (rfd, wfd) = nix::unistd::pipe().unwrap();
        let (tx, rx) = create_thread_command_channel();

        self.wfd = Some(wfd);
        self.tx = Some(tx);
        self.handle = Some(thread::spawn(move || {
            let bridge_locked = bridge;
            PlatformRs::run(bridge_locked, rfd, rx);
        }));
    }


    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}
    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn create_input_device(&self, device_id: i32) -> Box<InputDeviceRs> {
        Box::new(InputDeviceRs { device_id: device_id, wfd: self.wfd.as_ref().unwrap().as_fd(), tx: self.tx.clone().unwrap() })
    }

    fn run(bridge_locked: SharedPtr<PlatformBridgeC>, rfd: OwnedFd, rx: std::sync::mpsc::Receiver<ThreadCommand>) {
        let mut state = LibinputState {
            libinput: Libinput::new_with_udev(LibinputInterfaceImpl {
                bridge: bridge_locked.clone(),
                fds: Vec::new(),
            }),
            known_devices: Vec::new(),
            next_device_id: 0
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
            }
        ];

        loop {
            println!("Waiting for input events...");
            unsafe {
                let ret = poll(fds.as_mut_ptr(), fds.len() as _, -1); // -1 = wait indefinitely
                if ret <= 0 {
                    println!("Error polling libinput fd");
                    return;
                }
            }

            if state.libinput.dispatch().is_err() {
                // TODO: Report the error to Mir's logging facilities somehow
                println!("Error dispatching libinput events");
                return;
            }


            if fds[0].revents & POLLIN != 0 {
                println!("Processing input events");
                for event in &mut state.libinput {
                    println!("Got event: {:?}", event);

                    match event {
                        Event::Device(device_event) => {
                            match device_event  {
                                DeviceEvent::Added(added_event) => {
                                    let dev: Device = added_event.device();
                                    state.known_devices.push(DeviceWithId { id: state.next_device_id, device: dev });
                                    state.next_device_id += 1;
                                },
                                DeviceEvent::Removed(removed_event) => {
                                    let dev: Device  = removed_event.device();
                                    let index = state.known_devices.iter().position(|x| x.device.as_raw() == dev.as_raw());
                                    if let Some(index) = index {
                                        state.known_devices.remove(index);
                                    }
                                },
                                _ => {}
                            }
                        },

                        Event::Pointer(pointer_event) => {
                            println!("Pointer event: {:?}", pointer_event);
                        },

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
                            if let Some(dev_with_id) = state.known_devices.iter().find(|d| d.id == id) {
                                let info = InputDeviceInfoRs {
                                    name: dev_with_id.device.name().to_string(),
                                    unique_id: "TODO".to_string(),
                                    capabilities: 0
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
                    }
                }
            }
        }
    }
}

pub struct DeviceObserverRs {
    fd: Option<i32>
}

impl DeviceObserverRs {
    pub fn new() -> Self {
        DeviceObserverRs { fd: None }
    }

    pub fn activated(&mut self, fd: i32) {
        println!("Device activated with fd: {}", fd);
        self.fd = Some(fd);
    }

    pub fn suspended(&mut self, ) {
        println!("Device suspended");
    }

    pub fn removed(&mut self, ) {
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
    pub fn new(
        device_id: i32,
        wfd: BorrowedFd<'fd>,
        tx: mpsc::Sender<ThreadCommand>,) -> Self {
        InputDeviceRs { device_id, wfd, tx }
    }

    pub fn start(&mut self) {
        // Start reading events from the device and sending them to the sink
    }

    pub fn stop(&mut self) {
    }

    pub fn get_device_info(&self) -> Box<InputDeviceInfoRs> {
        let (tx, rx) = create_device_info_rs_channel();
        self.tx.send(ThreadCommand::GetDeviceInfo(self.device_id, tx)).unwrap();
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

        fn start(self: &mut InputDeviceRs);
        fn stop(self: &mut InputDeviceRs);
        fn get_device_info(self: &InputDeviceRs) -> Box<InputDeviceInfoRs>;

        fn name(self: &InputDeviceInfoRs) -> &str;
        fn unique_id(self: &InputDeviceInfoRs) -> &str;
        fn capabilities(self: &InputDeviceInfoRs) -> i32;

        fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>, device_registry: SharedPtr<InputDeviceRegistry>) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("/home/matthew/Github/mir/src/platforms/evdev-rs/platform_bridge.h");
        include!("/home/matthew/Github/mir/include/platform/mir/input/input_device_registry.h");

        type PlatformBridgeC;
        type DeviceBridgeC;

        #[namespace = "mir::input"]
        type InputDevice;

        #[namespace = "mir::input"]
        type InputDeviceRegistry;

        // // TODO: Add the device observer as well
        fn acquire_device(self: &PlatformBridgeC, major: i32, minor: i32) -> UniquePtr<DeviceBridgeC>;
        fn create_input_device(self: &PlatformBridgeC, device_id: i32) -> SharedPtr<InputDevice>;
        fn raw_fd(self: &DeviceBridgeC) -> i32;
    }
}

pub use ffi::PlatformBridgeC;
pub use ffi::DeviceBridgeC;
pub use ffi::InputDeviceRegistry;

pub fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>, device_registry: SharedPtr<InputDeviceRegistry>) -> Box<PlatformRs> {
    return Box::new(PlatformRs { bridge: bridge, handle: None, wfd: None, tx: None } );
}
