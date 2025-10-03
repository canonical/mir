use input::event::{DeviceEvent, EventTrait};
use input::{AsRaw, Device, Event, Libinput, LibinputInterface};
use std::os::fd::{AsRawFd, FromRawFd};
use std::path::Path;
use libc::{O_RDONLY, O_RDWR, O_WRONLY, c_int, dev_t, stat, major, minor, O_CLOEXEC};
use std::ffi::CString;
use std::os::unix::ffi::OsStrExt;
use std::fs::{File, OpenOptions};
use std::os::unix::{fs::OpenOptionsExt, io::OwnedFd};
use cxx::SharedPtr;

struct LibinputInterfaceImpl {
    bridge: SharedPtr<PlatformBridgeC>,
    fds: Vec<OwnedFd>,
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
        let fd = self.bridge.acquire_device(major_num as i32, minor_num as i32);
        println!("Acquired fd: {} for device with major: {}, minor: {}", fd, major_num, minor_num);
        let owned = unsafe { OwnedFd::from_raw_fd(fd) };
        self.fds.push(owned.try_clone().unwrap());
        println!("Owned fd: {}", owned.as_raw_fd().to_string());
        Ok(owned)
    }

    // This method is called when libinput is done with a device.
    fn close_restricted(&mut self, fd: OwnedFd) {
        println!("Closing device with fd: {}", fd.as_raw_fd().to_string());
        // drop(File::from(fd));
    }
}

pub struct PlatformRs {
    bridge: SharedPtr<PlatformBridgeC>,
    libinput: Option<Libinput>,
    known_devices: Vec<Device>
}

impl PlatformRs {
    pub fn start(&mut self) {
        println!("Starting evdev-rs platform");
        self.libinput = Some(Libinput::new_with_udev(LibinputInterfaceImpl{ bridge: self.bridge.clone(), fds: Vec::new() }));
        let libinput = self.libinput.as_mut().unwrap();
        libinput.udev_assign_seat("seat1").unwrap();
        self.process_input_events();
    }
    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}
    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    fn process_input_events(&mut self) {
        let libinput = self.libinput.as_mut().unwrap();
        if libinput.dispatch().is_err() {
            // TODO: Report the error to Mir's logging facilities somehow
            println!("Error dispatching libinput events");
            return;
        }


        println!("Processing libinput events");
        for event in libinput {
            println!("Got event: {:?}", event);

            match event {
                Event::Device(device_event) => {
                    match device_event  {
                        DeviceEvent::Added(added_event) => {
                            let dev: Device = added_event.device();
                            self.known_devices.push(dev);
                        },
                        DeviceEvent::Removed(removed_event) => {
                            let dev: Device  = removed_event.device();
                            let index = self.known_devices.iter().position(|x| x.as_raw() == dev.as_raw());
                            if let Some(index) = index {
                                self.known_devices.remove(index);
                            }
                        },
                        _ => {}
                    }
                },

                // TODO: Otherwise, find the event, process it, and request that the input sink handle it
                _ => {}
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

#[cxx::bridge(namespace = "mir::input::evdev_rs")]
mod ffi {

    extern "Rust" {
        type PlatformRs;
        type DeviceObserverRs;

        fn start(self: &mut PlatformRs);
        fn continue_after_config(self: &PlatformRs);
        fn pause_for_config(self: &PlatformRs);
        fn stop(self: &PlatformRs);
        fn create_device_observer(self: &PlatformRs) -> Box<DeviceObserverRs>;

        fn activated(self: &mut DeviceObserverRs, fd: i32);
        fn suspended(self: &mut DeviceObserverRs);
        fn removed(self: &mut DeviceObserverRs);

        fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("/home/matthew/Github/mir/src/platforms/evdev-rs/platform_bridge.h");

        type PlatformBridgeC;
        // type DeviceC;

        // // TODO: Add the device observer as well
        fn acquire_device(&self, major: i32, minor: i32) -> i32;
    }
}

pub use ffi::PlatformBridgeC;

pub fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs> {
    return Box::new(PlatformRs { bridge: bridge, libinput: None, known_devices: Vec::new() });
}
