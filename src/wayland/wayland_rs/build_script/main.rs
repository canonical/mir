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
mod protocol_parser;

use cpp_builder::{
    CppArg, CppBuilder, CppClass, CppEnum, CppEnumOption, CppMethod, CppNamespace, CppType,
};
use proc_macro2::TokenStream;
use protocol_parser::{
    parse_protocols, WaylandEnum, WaylandInterface, WaylandProtocol, WaylandRequest,
};
use quote::{format_ident, quote};
use std::fs;
use syn::Ident;

fn main() {
    cxx_build::bridges(vec!["src/lib.rs"]).compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");

    // First, parse the protocol XML files.
    let protocols: Vec<WaylandProtocol> = parse_protocols();

    // Next, generate the protocols.rs file.
    write_protocols_rs(&protocols);

    // Next, generate the dispatch and global dispatch methods.
    write_dispatch_rs(&protocols);

    // Next, generate a C++ abstract class for each interface.
    write_cpp_protocol_headers(&protocols);
}

fn write_protocols_rs(protocols: &Vec<WaylandProtocol>) {
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

    let generated_protocol_rs = quote! {
        use wayland_server;

        #(#generated_protocols)*
    };

    write_generated_rust_file(generated_protocol_rs, "protocols.rs");
}

/// Generate the namespace token for a protocol (either wayland_server::protocol or protocols::module_name).
fn generate_namespace(protocol: &WaylandProtocol) -> TokenStream {
    if protocol.name == "wayland" {
        quote! { wayland_server::protocol }
    } else {
        let protocol_module = dash_to_snake_ident(&protocol.name);
        quote! { protocols::#protocol_module }
    }
}

/// Generate a GlobalDispatch implementation for a single interface.
fn generate_global_dispatch_impl(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name.to_string());
    let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));

    if interface_name == "wl_display" {
        // wl_display is handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    quote! {
        impl GlobalDispatch<#namespace_name::#interface_name::#interface_struct_name, ()>
            for ServerState
        {
            fn bind(
                _state: &mut Self,
                _handle: &wayland_server::DisplayHandle,
                _client: &wayland_server::Client,
                resource: New<#namespace_name::#interface_name::#interface_struct_name>,
                _global_data: &(),
                data_init: &mut wayland_server::DataInit<'_, Self>,
            ) {
                data_init.init(resource, ());
            }
        }
    }
}

/// Generate request handler arms for an interface's requests.
fn generate_request_handler_arms(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
    interface_name: &Ident,
) -> Vec<TokenStream> {
    interface
        .items
        .iter()
        .filter_map(|item| {
            if let protocol_parser::InterfaceItem::Request(request) = item {
                let request_name = format_ident!("{}", snake_to_pascal(&request.name));
                let arg_names: Vec<proc_macro2::TokenStream> = request
                    .args
                    .iter()
                    .map(|arg| {
                        let arg_name = format_ident!("{}", arg.name.replace('-', "_"));
                        quote! { #arg_name: _ }
                    })
                    .collect();

                Some(quote! {
                    #namespace_name::#interface_name::Request::#request_name { #( #arg_names ),* } => {
                        // Handle the #request_name request here
                    }
                })
            } else {
                None
            }
        })
        .collect()
}

/// Generate a Dispatch implementation for a single interface.
fn generate_dispatch_impl(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
    is_wayland_protocol: bool,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name);

    if interface_name == "wl_display" || interface_name == "wl_registry" {
        // wl_display and wl_registry are handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));

    let mut request_handler_arms =
        generate_request_handler_arms(interface, namespace_name, &interface_name);

    // If the interface comes from wayland, we need to generate an empty arm because the
    // enum is marked as non-exhaustive.
    if is_wayland_protocol {
        request_handler_arms.push(quote! {
            _ => {}
        });
    }

    quote! {
        impl Dispatch<#namespace_name::#interface_name::#interface_struct_name, ()>
            for ServerState
        {
            fn request(
                _state: &mut Self,
                _client: &Client,
                _resource: &#namespace_name::#interface_name::#interface_struct_name,
                request: <#namespace_name::#interface_name::#interface_struct_name as wayland_server::Resource>::Request,
                _data: &(),
                _dhandle: &DisplayHandle,
                _data_init: &mut DataInit<'_, Self>,
            ) {
                match request {
                    #(#request_handler_arms),*
                }
            }
        }
    }
}

/// Generate all dispatch implementations (both GlobalDispatch and Dispatch) for a single protocol.
fn generate_dispatch_implementations(protocol: &WaylandProtocol) -> TokenStream {
    let namespace_name = generate_namespace(protocol);

    let global_interfaces: Vec<&protocol_parser::WaylandInterface> = protocol
        .interfaces
        .iter()
        .filter(|interface| interface.is_global)
        .collect();

    let global_dispatch_impls = global_interfaces
        .iter()
        .map(|interface| generate_global_dispatch_impl(interface, &namespace_name));

    let is_wayland_protocol = protocol.name == "wayland";
    let dispatch_impls = protocol
        .interfaces
        .iter()
        .map(|interface| generate_dispatch_impl(interface, &namespace_name, is_wayland_protocol));

    quote! {
        #(#global_dispatch_impls)*
        #(#dispatch_impls)*
    }
}

fn write_dispatch_rs(protocols: &Vec<WaylandProtocol>) {
    let generated_dispatch_implementations = protocols
        .iter()
        .map(|protocol| generate_dispatch_implementations(protocol));

    let generated_protocol_rs = quote! {
        use wayland_server::{Client, DataInit, Dispatch, GlobalDispatch, New, DisplayHandle};
        use crate::protocols;
        use crate::wayland_server::ServerState;

        #(#generated_dispatch_implementations)*
    };

    write_generated_rust_file(generated_protocol_rs, "dispatch.rs");
}

/// Write a header file for each protocol containing abstract classes per-interface.
fn write_cpp_protocol_headers(protocols: &Vec<WaylandProtocol>) {
    protocols
        .iter()
        .for_each(|protocol| write_cpp_protocol_header(protocol));
}

fn write_cpp_protocol_header(protocol: &WaylandProtocol) {
    let guard = format!("MIR_WAYLANDRS_{}", protocol.name.to_uppercase());
    let mut builder = CppBuilder::new(guard);
    let mut namespace = CppNamespace::new(vec!["mir".to_string(), "wayland_rs".to_string()]);

    let classes = protocol
        .interfaces
        .iter()
        .map(|interface| wayland_interface_to_cpp_class(interface));

    for class in classes {
        namespace.add_class(class);
    }
    builder.add_namespace(namespace);
    builder.add_include("<rust/cxx.h>".to_string());

    for interface in &protocol.dependencies {
        builder.add_forward_declaration_class(&snake_to_pascal(interface));
    }

    let filename = format!("{}.h", protocol.name);
    write_generated_cpp_file(&builder.to_string(), filename.as_str());
}

fn wayland_interface_to_cpp_class(interface: &WaylandInterface) -> CppClass {
    let mut class = CppClass::new(snake_to_pascal(&interface.name));
    let methods = interface.items.iter().filter_map(|item| {
        if let protocol_parser::InterfaceItem::Request(request) = item {
            Some(wayland_request_to_cpp_method(request))
        } else {
            None
        }
    });

    for enum_ in wayland_interface_to_enums(interface) {
        class.add_enum(enum_);
    }

    for method in methods {
        class.add_method(method);
    }

    class
}

fn wayland_interface_to_enums(interface: &WaylandInterface) -> impl Iterator<Item = CppEnum> + '_ {
    interface.items.iter().filter_map(|item| {
        if let protocol_parser::InterfaceItem::Enum(enum_) = item {
            Some(wayland_enum_to_cpp_enum(enum_))
        } else {
            None
        }
    })
}

fn wayland_enum_to_cpp_enum(enum_: &WaylandEnum) -> CppEnum {
    let mut result = CppEnum::new(snake_to_pascal(enum_.name.as_str()));

    for option in &enum_.entries {
        result.add_option(CppEnumOption {
            name: snake_to_pascal(option.name.as_str()),
            value: option.value as u32,
        });
    }

    result
}

fn wayland_request_to_cpp_method(method: &WaylandRequest) -> CppMethod {
    let mut cpp_method = CppMethod::new(method.name.clone());
    let args = method.args.iter().map(|arg| {
        let type_ = match arg.type_.as_str() {
            "int" => CppType::CppI32,
            "uint" => CppType::CppU32,
            "fixed" => CppType::CppF32,
            "string" => CppType::String,
            "object" => CppType::Object(snake_to_pascal(
                arg.interface
                    .clone()
                    .expect("Object is missing interface")
                    .as_str(),
            )),
            "new_id" => CppType::NewId(if let Some(interface) = arg.interface.clone() {
                Some(snake_to_pascal(interface.as_str()))
            } else {
                None
            }), // Note: WlRegistry allows for a bind without a defined interface
            "array" => CppType::Array,
            "fd" => CppType::Fd,
            _ => panic!("Unknown type: {}", arg.type_),
        };

        CppArg::new(type_, arg.name.clone())
    });

    for arg in args {
        cpp_method.add_arg(arg);
    }

    cpp_method
}

/// Write the generated Rust code to a file with proper formatting.
fn write_generated_rust_file(tokens: proc_macro2::TokenStream, filename: &str) {
    let out_dir = "src";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    let syntax_tree: syn::File = syn::parse2(tokens).unwrap();
    let formatted_code = prettyplease::unparse(&syntax_tree);

    fs::write(dest_path, formatted_code).unwrap();
}

/// Write the generated C++ code to the correct directory.
fn write_generated_cpp_file(content: &str, filename: &str) {
    let out_dir = "include";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    fs::create_dir_all(&out_dir).unwrap();
    fs::write(dest_path, content).unwrap();
}

fn dash_to_snake(name: &str) -> String {
    name.replace('-', "_")
}

fn dash_to_snake_ident(name: &str) -> Ident {
    format_ident!("{}", dash_to_snake(name))
}

fn snake_to_pascal(s: &str) -> String {
    s.split('_')
        .map(|word| {
            let mut chars = word.chars();
            match chars.next() {
                None => String::new(),
                Some(first) => first.to_uppercase().collect::<String>() + chars.as_str(),
            }
        })
        .collect()
}
