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

use crate::helpers::dash_to_snake_ident;
use crate::protocol_parser::WaylandProtocol;
use proc_macro2::TokenStream;
use quote::quote;

pub fn generate_protocols_rs(protocols: &Vec<WaylandProtocol>) -> TokenStream {
    let generated_protocols = protocols.iter().map(|protocol| {
        // We rely on the wayland_server crate for the core Wayland protocol.
        if protocol.name == "wayland" {
            return quote! {};
        }

        let struct_name = dash_to_snake_ident(&protocol.name);
        let path = &protocol.path;

        // Add use statements for other protocol dependencies.
        let protocol_dependencies: std::collections::HashSet<String> = protocol
            .dependencies
            .iter()
            .filter_map(|dependency| {
                protocols
                    .iter()
                    .find(|dep_protocol| {
                        dep_protocol.name != protocol.name
                            && dep_protocol
                                .interfaces
                                .iter()
                                .any(|iface| iface.name == *dependency)
                    })
                    .map(|dep_protocol| dep_protocol.name.clone())
            })
            .collect();

        let mut server_code_use_statements = quote! {
            use super::wayland_server;
            use self::interfaces::*;
        };

        for dependency in &protocol_dependencies {
            let dep_struct_name = dash_to_snake_ident(dependency);
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
            let dep_struct_name = dash_to_snake_ident(dependency);
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
                        use wayland_server::protocol::__interfaces::*;
                    };
                } else {
                    // Note: #[allow(unused_imports)] is used here to suppress warnings in case some protocols
                    // do not actually use any interfaces from their dependencies. This only happens in a
                    // input-method-unstable-v2 because it uses the enums from text-input-unstable-v3 but not
                    // the interface name or anything else.
                    //
                    // This is much simpler than piping the data through the protocol parser, so let's keep things
                    // straightforward for now.
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements

                        #[allow(unused_imports)]
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

    quote! {
        #[allow(dead_code, unused_imports)]
        mod protocols {
            use wayland_server;

            #(#generated_protocols)*
        }
    }
}
