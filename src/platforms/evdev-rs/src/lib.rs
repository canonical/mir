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


struct PlatformRsSharedState {
    libinput: Libinput,
    known_devices: Vec<Device>
}

pub struct PlatformRs {
    bridge: SharedPtr<PlatformBridgeC>,
    handle: Option<JoinHandle<()>>,
}

impl PlatformRs {
    pub fn start(&mut self) {
        println!("Starting evdev-rs platform");

        let bridge = self.bridge.clone(); // Arc clone, cheap, Send

        self.handle = Some(thread::spawn(move || {
            // We can lock the bridge inside the thread
            println!("Libinput started in thread!");
            let bridge_locked = bridge;
            PlatformRs::process_input_events(bridge_locked);
        }));
    }


    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}
    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn process_input_events(bridge_locked: SharedPtr<PlatformBridgeC>) {
        let mut state = PlatformRsSharedState {
            libinput: Libinput::new_with_udev(LibinputInterfaceImpl {
                bridge: bridge_locked.clone(),
                fds: Vec::new(),
            }),
            known_devices: Vec::new(),
        };

        state.libinput.udev_assign_seat("seat0").unwrap();

        let libinput_fd = state.libinput.as_raw_fd();

        loop {
            println!("Waiting for input events...");
            let mut fds = [pollfd {
                fd: libinput_fd,
                events: POLLIN, 
                revents: 0,
            }];

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
                        match pointer_event {
                            input::event::pointer::PointerEvent::MotionAbsolute(pointer_motion_absolute_event) => {
                            },
                            // | input::event::pointer::PointerEvent::Motion(_)
                            // | input::event::pointer::PointerEvent::Button(_)
                            // | input::event::pointer::PointerEvent::Axis(_) => {},
                            _ => {}
                        }
                    },
    
                    // TODO: Otherwise, find the event, process it, and request that the input sink handle it
                    _ => {}
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

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        #[namespace = "mir::input::evdev_rs"]
        type PlatformRs;

        #[namespace = "mir::input::evdev_rs"]
        type DeviceObserverRs;

        #[namespace = "mir::input::evdev_rs"]
        fn start(self: &mut PlatformRs);

        #[namespace = "mir::input::evdev_rs"]
        fn continue_after_config(self: &PlatformRs);

        #[namespace = "mir::input::evdev_rs"]
        fn pause_for_config(self: &PlatformRs);

        #[namespace = "mir::input::evdev_rs"]
        fn stop(self: &PlatformRs);

        #[namespace = "mir::input::evdev_rs"]
        fn create_device_observer(self: &PlatformRs) -> Box<DeviceObserverRs>;

        #[namespace = "mir::input::evdev_rs"]
        fn activated(self: &mut DeviceObserverRs, fd: i32);

        #[namespace = "mir::input::evdev_rs"]
        fn suspended(self: &mut DeviceObserverRs);

        #[namespace = "mir::input::evdev_rs"]
        fn removed(self: &mut DeviceObserverRs);

        #[namespace = "mir::input::evdev_rs"]
        fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("/home/matthew/Github/mir/src/platforms/evdev-rs/platform_bridge.h");

        #[namespace = "mir::input::evdev_rs"]
        type PlatformBridgeC;

        #[namespace = "mir::input::evdev_rs"]
        type DeviceBridgeC;

        #[namespace = "mir::input"]
        type InputDeviceRegistry;

        #[namespace = "mir::input::evdev_rs"]
        fn input_device_registry(self: &PlatformBridgeC) -> SharedPtr<InputDeviceRegistry>;

        #[namespace = "mir::input::evdev_rs"]
        fn acquire_device(self: &PlatformBridgeC, major: i32, minor: i32) -> UniquePtr<DeviceBridgeC>;

        #[namespace = "mir::input::evdev_rs"]
        fn raw_fd(self: &DeviceBridgeC) -> i32;
    }
}

pub use ffi::PlatformBridgeC;
pub use ffi::DeviceBridgeC;

pub fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs> {
    return Box::new(PlatformRs { bridge: bridge, handle: None } );
}
