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

mod cpp_builder;
mod cpp_protocol_generation;
mod dispatch_generator;
mod ffi_generation;
mod helpers;
mod protocol_generator;
mod protocol_middleware_generation;
mod protocol_parser;
mod wayland_server_generation;

use cpp_builder::CppBuilder;
use cpp_protocol_generation::generate_cpp_protocol_builders;
use ffi_generation::generate_ffi;
use helpers::*;
use protocol_middleware_generation::generate_wayland_interface_middleware;
use protocol_parser::{parse_protocols, WaylandProtocol};
use std::{env, path::Path};

use crate::dispatch_generator::generate_dispatch_rs;
use crate::protocol_generator::generate_protocols_rs;
use crate::wayland_server_generation::generate_wayland_server_generated_rs;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let include_path = Path::new(&manifest_dir).join(".");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server_core.rs");
    println!("cargo:rerun-if-changed=build_script/ffi_generation.rs");
    println!("cargo:rerun-if-changed=build_script/cpp_protocol_generation.rs");
    println!("cargo:rerun-if-changed=build_script/dispatch_generator.rs");
    println!("cargo:rerun-if-changed=build_script/protocol_generator.rs");
    println!("cargo:rerun-if-changed=build_script/protocol_middleware_generation.rs");
    println!("cargo:rerun-if-changed=build_script/wayland_server_generation.rs");

    // First, parse the protocol XML files.
    let protocols: Vec<WaylandProtocol> = parse_protocols();

    // Next, generate the protocols.rs file.
    write_protocols_rs(&protocols);

    // Next, generate the dispatch and global dispatch methods.
    write_dispatch_rs(&protocols);

    // Next, generate the protocol middleware classes.
    write_protocol_middleware(&protocols);

    // Next, generate C++ abstract classes for each interface
    // as well as the FFI code.
    write_cpp_protocol_implementations(&protocols);

    // Next, write the generated side of the WaylandServer module.
    write_wayland_server_generated(&protocols);

    // Finally, declare the bridges.
    // This must happen last because `src/ffi.rs` is built by this script and
    // it may not exist before the build is run.
    cxx_build::bridges(vec!["src/ffi.rs"])
        .include(&include_path)
        .compile("wayland_rs");
}

fn write_protocols_rs(protocols: &Vec<WaylandProtocol>) {
    let tokens = generate_protocols_rs(protocols);
    write_generated_rust_file(tokens, "protocols.rs");
}

fn write_dispatch_rs(protocols: &Vec<WaylandProtocol>) {
    let tokens = generate_dispatch_rs(protocols);
    write_generated_rust_file(tokens, "dispatch.rs");
}

fn write_protocol_middleware(protocols: &Vec<WaylandProtocol>) {
    let middleware = generate_wayland_interface_middleware(protocols);
    write_generated_rust_file(middleware, "middleware.rs");
}

/// Write a header file for each protocol containing abstract classes per-interface.
fn write_cpp_protocol_implementations(protocols: &Vec<WaylandProtocol>) {
    let output = generate_cpp_protocol_builders(protocols);

    // Generate ffi_fwd.h first so that protocol headers can include it without
    // creating a circular dependency with the CXX-generated ffi.rs.h.
    write_cpp_header(&output.ffi_fwd_builder);

    // Write the protocol headers
    for builder in &output.builders {
        write_cpp_header(&builder);
        write_cpp_source(&builder);
    }

    let ffi = generate_ffi(&protocols, &output.builders);
    write_generated_rust_file(ffi, "ffi.rs");
}

fn write_cpp_header(builder: &CppBuilder) {
    let filename = format!("{}.h", builder.filename);
    write_generated_cpp_file(
        &builder.to_cpp_header(),
        "wayland_rs_cpp/include",
        filename.as_str(),
    );
}

fn write_cpp_source(builder: &CppBuilder) {
    let header_filename = format!("{}.h", builder.filename);

    let filename = format!("{}.cpp", builder.filename);
    write_generated_cpp_file(
        &builder.to_cpp_source(&header_filename),
        "wayland_rs_cpp/src",
        filename.as_str(),
    );
}

fn write_wayland_server_generated(protocols: &Vec<WaylandProtocol>) {
    let tokens = generate_wayland_server_generated_rs(&protocols);
    write_generated_rust_file(tokens, "wayland_server_generated.rs");
}
