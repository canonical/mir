use input::event::{DeviceEvent, EventTrait};
use input::{AsRaw, Device, Event, Libinput, LibinputInterface};
use std::os::fd::{AsRawFd, FromRawFd};
use std::path::Path;
use libc::{stat, major, minor, poll, pollfd, POLLIN};
use std::ffi::CString;
use std::os::unix::ffi::OsStrExt;
use std::os::unix::{io::OwnedFd};
use cxx::SharedPtr;
use cxx::UniquePtr;
use std::thread::{self, JoinHandle};
use nix::unistd::close;

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


struct LibinputState {
    libinput: Libinput,
    known_devices: Vec<Device>
}

pub struct PlatformRs {
    bridge: SharedPtr<PlatformBridgeC>,
    handle: Option<JoinHandle<()>>,
    wfd: Option<OwnedFd>,
    tx: Option<std::sync::mpsc::Sender<LibinputCommand>>,
}

enum LibinputCommandType {
    Shutdown,
}

struct LibinputCommand {
    view_id: i32,
    command_type: LibinputCommandType,
}

impl PlatformRs {
    pub fn start(&mut self) {
        println!("Starting evdev-rs platform");

        let bridge = self.bridge.clone(); // Arc clone, cheap, Send
        let (rfd, wfd) = nix::unistd::pipe().unwrap();
        let (tx, rx) = std::sync::mpsc::channel<LibinputCommand>();

        self.wfd = Some(wfd);
        self.tx = Some(tx);
        self.handle = Some(thread::spawn(move || {
            // We can lock the bridge inside the thread
            println!("Libinput started in thread!");
            let bridge_locked = bridge;
            PlatformRs::process_input_events(bridge_locked, rfd, rx);
        }));
    }


    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}
    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn create_input_device(&self) -> Box<InputDeviceRs> {
        Box::new(InputDeviceRs { })
    }

    fn process_input_events(bridge_locked: SharedPtr<PlatformBridgeC>, rfd: OwnedFd, receiver: std::sync::mpsc::Receiver<LibinputCommand>) {
        let mut state = LibinputState {
            libinput: Libinput::new_with_udev(LibinputInterfaceImpl {
                bridge: bridge_locked.clone(),
                fds: Vec::new(),
            }),
            known_devices: Vec::new(),
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
                                    state.known_devices.push(dev);
                                },
                                DeviceEvent::Removed(removed_event) => {
                                    let dev: Device  = removed_event.device();
                                    let index = state.known_devices.iter().position(|x| x.as_raw() == dev.as_raw());
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
                nix::unistd::read(rfd, &mut buf).unwrap();
                // handle commands
                while let Ok(cmd) = rx.try_recv() {
                    match cmd.command_type {
                        LibinputCommandType::DoSomething => { /* ... */ }
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

pub struct InputDeviceRs {
    // device: Device
}

impl InputDeviceRs {
    // pub fn new(device: Device) -> Self {
    //     InputDeviceRs { device }
    // }
}

#[cxx::bridge(namespace = "mir::input::evdev_rs")]
mod ffi {
    extern "Rust" {
        type PlatformRs;
        type DeviceObserverRs;
        type InputDeviceRs;

        fn start(self: &mut PlatformRs);
        fn continue_after_config(self: &PlatformRs);
        fn pause_for_config(self: &PlatformRs);
        fn stop(self: &PlatformRs);
        fn create_device_observer(self: &PlatformRs) -> Box<DeviceObserverRs>;
        fn create_input_device(self: &PlatformRs) -> Box<InputDeviceRs>;

        fn activated(self: &mut DeviceObserverRs, fd: i32);
        fn suspended(self: &mut DeviceObserverRs);
        fn removed(self: &mut DeviceObserverRs);

        fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("/home/matthew/Github/mir/src/platforms/evdev-rs/platform_bridge.h");

        type PlatformBridgeC;
        type DeviceBridgeC;

        // // TODO: Add the device observer as well
        fn acquire_device(self: &PlatformBridgeC, major: i32, minor: i32) -> UniquePtr<DeviceBridgeC>;
        fn raw_fd(self: &DeviceBridgeC) -> i32;
    }
}

pub use ffi::PlatformBridgeC;
pub use ffi::DeviceBridgeC;

pub fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs> {
    return Box::new(PlatformRs { bridge: bridge, handle: None, wfd: None, tx: None } );
}
