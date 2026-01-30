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

mod protocol_parser;

use proc_macro2::TokenStream;
use protocol_parser::{parse_protocols, WaylandProtocol, WaylandProtocolMetadata};
use quote::{format_ident, quote};
use std::fs;

fn main() {
    cxx_build::bridges(vec!["src/lib.rs"]).compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");

    // First, parse the protocol XML files.
    let protocols: Vec<(WaylandProtocolMetadata, WaylandProtocol)> = parse_protocols();

    // Next, generate the protocols.rs file.
    write_protocols_rs(&protocols);
}

fn write_protocols_rs(protocols: &Vec<(WaylandProtocolMetadata, WaylandProtocol)>) {
    let generated_protocols = protocols.iter().map(|(metadata, protocol)| {
        // We rely on the wayland_server crate for the core Wayland protocol.
        if protocol.name == "wayland" {
            return quote! {};
        }

        let struct_name = format_ident!("{}", protocol.name.replace('-', "_"));
        let path = &metadata.path;

        let mut server_code_use_statements = quote! {
            use super::wayland_server;
            use self::interfaces::*;
        };

        // Add use statements for other protocol dependencies
        let protocol_dependencies: std::collections::HashSet<String> = metadata
            .dependencies
            .iter()
            .filter_map(|dependency| {
                protocols
                    .iter()
                    .find(|(_, dep_protocol)| {
                        dep_protocol.name != protocol.name
                            && dep_protocol
                                .interfaces
                                .iter()
                                .any(|iface| iface.name == *dependency)
                    })
                    .map(|(_, dep_protocol)| dep_protocol.name.clone())
            })
            .collect();

        for dependency in &protocol_dependencies {
            let dep_struct_name = format_ident!("{}", dependency.replace('-', "_"));
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    server_code_use_statements = quote! {
                        #server_code_use_statements
                        use wayland_server::protocol::*;
                    };
                } else {
                    server_code_use_statements = quote! {
                        #server_code_use_statements
                        use super::#dep_struct_name::*;
                    };
                }
            }
        }

        let mut interface_code_use_statements: TokenStream = quote! {};
        for dependency in &protocol_dependencies {
            let dep_struct_name = format_ident!("{}", dependency.replace('-', "_"));
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
                        use wayland_server::protocol::__interfaces::*;
                    };
                } else {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
                        use super::super::#dep_struct_name::interfaces::*;
                    };
                }
            }
        }

        quote! {
            pub mod #struct_name {

                #server_code_use_statements

                pub mod interfaces {
                    #interface_code_use_statements
                    wayland_scanner::generate_interfaces!(#path);
                }

                wayland_scanner::generate_server_code!(#path);
            }
        }
    });

    let generated_protocol_rs = quote! {
        use wayland_scanner;
        use wayland_server;

        #(#generated_protocols)*
    };

    write_generated_rust_file(generated_protocol_rs, "protocols.rs");
}

/// Write the generated Rust code to a file with proper formatting.
fn write_generated_rust_file(tokens: proc_macro2::TokenStream, filename: &str) {
    let out_dir = "src";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    let syntax_tree: syn::File = syn::parse2(tokens).unwrap();
    let formatted_code = prettyplease::unparse(&syntax_tree);

    fs::write(dest_path, formatted_code).unwrap();
}
