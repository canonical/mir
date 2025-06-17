#[cxx::bridge]
mod ffi {
  extern "Rust" {
    fn rust_println(s: &str);
  }
}

pub fn rust_println(s: &str) {
    println!("Printed from rust: {}", s);
}
