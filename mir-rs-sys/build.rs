/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

use std::env;
use std::path::PathBuf;

fn main() {
    let miral = pkg_config::Config::new()
        .atleast_version("5.0")
        .probe("miral")
        .expect(
            "\n\n\
            ERROR: Could not find the miral C++ library.\n\
            \n\
            The mir-sys crate requires the system-installed miral library.\n\
            \n\
            On Ubuntu/Debian:\n\
            \n\
            sudo apt install libmiral-dev\n\
            \n\
            For other systems, see: https://github.com/canonical/mir/blob/main/HACKING.md\n\
            \n",
        );

    let mir_core = pkg_config::Config::new()
        .atleast_version("2.0")
        .probe("mircore")
        .expect("Could not find mircore. Install libmir-core-dev.");

    let mir_common = pkg_config::Config::new()
        .probe("mircommon")
        .expect("Could not find mircommon. Install libmircommon-dev.");

    // Gather include paths from pkg-config
    let include_paths: Vec<PathBuf> = miral
        .include_paths
        .iter()
        .chain(mir_core.include_paths.iter())
        .chain(mir_common.include_paths.iter())
        .cloned()
        .collect();

    // --- bindgen: Generate Rust bindings for C-linkage enums ---
    let mut bindgen_builder = bindgen::Builder::default()
        .header_contents(
            "bindgen_input.h",
            "#include <mir_toolkit/common.h>\n#include <mir_toolkit/events/enums.h>\n",
        )
        .allowlist_type("Mir.*")
        .allowlist_var("mir_.*")
        .rustified_enum("Mir.*")
        .derive_debug(true)
        .derive_copy(true)
        .derive_eq(true)
        .derive_hash(true);

    for path in &include_paths {
        bindgen_builder = bindgen_builder.clang_arg(format!("-I{}", path.display()));
    }

    let bindings = bindgen_builder
        .generate()
        .expect("Failed to generate bindgen bindings for mir_toolkit enums");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("mir_enums.rs"))
        .expect("Failed to write bindgen output");

    // --- cxx-build: Build the C++ bridge ---
    let mut cxx_build = cxx_build::bridge("src/lib.rs");

    for path in &include_paths {
        cxx_build.include(path);
    }

    // Add the src directory so generated CXX code can find bridge.h
    cxx_build.include("src");

    cxx_build
        .file("src/bridge.cpp")
        .std("c++23")
        .compile("mir_sys_bridge");

    // Link against miral and its dependencies
    for lib in &miral.libs {
        println!("cargo:rustc-link-lib={}", lib);
    }
    for path in &miral.link_paths {
        println!("cargo:rustc-link-search=native={}", path.display());
    }

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=src/bridge.h");
}
