fn main() {
    cxx_build::bridges(vec!["src/lib.rs"]).compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");
}
