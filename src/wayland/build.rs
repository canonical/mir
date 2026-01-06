fn main() {
    cxx_build::bridge("src/lib.rs")
        .compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
}
