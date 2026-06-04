fn main() {
    let _ = cxx_build::bridge("src/lib.rs");

    // Rebuild whenever any Rust source file changes.
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/device.rs");
    println!("cargo:rerun-if-changed=src/event_processing.rs");
    println!("cargo:rerun-if-changed=src/ffi.rs");
    println!("cargo:rerun-if-changed=src/libinput_interface.rs");
    println!("cargo:rerun-if-changed=src/platform.rs");
    println!("cargo:rerun-if-changed=src/udev_monitor.rs");
}
