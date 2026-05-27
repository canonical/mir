fn main() {
    let _ = cxx_build::bridge("src");
    println!("cargo:rerun-if-changed=src/lib.rs");
}
