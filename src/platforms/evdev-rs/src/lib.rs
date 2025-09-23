#[cxx::bridge]
mod ffi {
  extern "Rust" {
    fn evdev_rs_start();
    fn evdev_rs_continue_after_config();
    fn evdev_rs_pause_for_config();
    fn evdev_rs_stop();
  }
}

pub fn evdev_rs_start() {
    println!("Starting evdev input platform");
}

pub fn evdev_rs_continue_after_config() {
    println!("Continuing evdev input platform");
}

pub fn evdev_rs_pause_for_config() {
    println!("Pausing evdev input platform");
}

pub fn evdev_rs_stop() {
    println!("Stopping evdev input platform");
}
