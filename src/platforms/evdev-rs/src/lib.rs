use input::event::{DeviceEvent, EventTrait};
use input::{AsRaw, Device, Event, Libinput, LibinputInterface};
use std::path::Path;
use libc::{O_RDONLY, O_RDWR, O_WRONLY};
use std::fs::{File, OpenOptions};
use std::os::unix::{fs::OpenOptionsExt, io::OwnedFd};
use cxx::SharedPtr;

struct LibinputInterfaceImpl;

impl LibinputInterface for LibinputInterfaceImpl {
    // This method is called whenever libinput needs to open a new device.
    fn open_restricted(&mut self, path: &Path, flags: i32) -> Result<OwnedFd, i32> {
       OpenOptions::new()
            .custom_flags(flags)
            .read((flags & O_RDONLY != 0) | (flags & O_RDWR != 0))
            .write((flags & O_WRONLY != 0) | (flags & O_RDWR != 0))
            .open(path)
            .map(|file| file.into())
            .map_err(|err| err.raw_os_error().unwrap())
    }

    // This method is called when libinput is done with a device.
    fn close_restricted(&mut self, fd: OwnedFd) {
        drop(File::from(fd));
    }
}

pub struct PlatformRs {
    libinput: Option<Libinput>,
    known_devices: Vec<Device>
}

impl PlatformRs {
    pub fn start(&mut self) {
        self.libinput = Some(Libinput::new_with_udev(LibinputInterfaceImpl));
        self.process_input_events();
    }
    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}
    pub fn stop(&self) {}

    fn process_input_events(&mut self) {
        let libinput = self.libinput.as_mut().unwrap();
        if libinput.dispatch().is_err() {
            // TODO: Report the error to Mir's logging facilities somehow
            return;
        }

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

#[cxx::bridge]
mod ffi {

    extern "Rust" {
        type PlatformRs;

        fn start(self: &mut PlatformRs);
        fn continue_after_config(self: &PlatformRs);
        fn pause_for_config(self: &PlatformRs);
        fn stop(self: &PlatformRs);

        fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("/home/matthew/Github/mir/src/platforms/evdev-rs/platform_bridge.h");

        type PlatformBridgeC;
        // type DeviceC;

        // // TODO: Add the device observer as well
        // fn acquire_device(major: i32, minor: i32) -> UniquePtr<DeviceC>;
    }
}

pub use ffi::PlatformBridgeC;

pub fn evdev_rs_create(bridge: SharedPtr<PlatformBridgeC>) -> Box<PlatformRs> {
    return Box::new(PlatformRs { libinput: None, known_devices: Vec::new() });
}
