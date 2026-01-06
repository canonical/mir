fn start() {}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn start();
    }
}
