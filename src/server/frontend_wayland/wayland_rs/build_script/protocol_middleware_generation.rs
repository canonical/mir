//! # Protocol middleware code generation
//!
//! This module provides a method for generating middleware classes
//! for each protocol. These classes ferry the data from C++ to Rust
//! so that the interface at the ffi barrier is clean.
//!
//! Each C++ interface will host a Box<...> of one of these extensions
//! that it will use to call into the Rust code.
//!
//! use crate::cpp_builder::CppBuilder;

use crate::cpp_builder::sanitize_identifier;
use crate::helpers::format_wayland_interface_to_rust_extension_struct;

use super::helpers::snake_to_pascal;
use super::{
    InterfaceItem, WaylandArg, WaylandArgType, WaylandEvent, WaylandInterface, WaylandProtocol,
};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};

pub fn generate_wayland_interface_middleware(protocols: &Vec<WaylandProtocol>) -> TokenStream {
    // First, generate the imports.
    let use_imports: Vec<TokenStream> = protocols
        .iter()
        .flat_map(generate_use_imports_for_protocol)
        .collect();

    // Next, generate the C++ -> Rust side.
    //
    // Note that there is a complication here. Certain arguments will not
    // be able to be ferried directly from C++ to Rust. This is because the Rust
    // Wayland protocol methods take arguments that cannot be sent over the C++ -> Rust
    // boundary. To fix this, we first generate middleware structs that make it easy
    // to copy the data.
    let extensions = protocols.iter().flat_map(generate_extensions_for_protocol);

    quote! {
        #[allow(dead_code, unused_imports)]
        mod middleware {
            use crate::wayland_server_core::*;
            use wayland_server::protocol::*;
            use cxx::{CxxString, CxxVector};
            use std::os::unix::io::BorrowedFd;
            #(#use_imports)*

            #(#extensions)*
        }
    }
}

fn generate_use_imports_for_protocol(protocol: &WaylandProtocol) -> Vec<TokenStream> {
    let is_core_wayland = protocol.name == "wayland";
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .flat_map(|interface| {
            let (interface_struct_path, interface_module_path) = if is_core_wayland {
                let struct_path: syn::Path = syn::parse_str(&format!(
                    "wayland_server::protocol::{}::{}",
                    interface.name,
                    snake_to_pascal(&interface.name)
                ))
                .expect("Failed to parse full interface path");

                let module_path: syn::Path =
                    syn::parse_str(&format!("wayland_server::protocol::{}", interface.name,))
                        .expect("Failed to parse interface module path");

                (struct_path, module_path)
            } else {
                let struct_path: syn::Path = syn::parse_str(&format!(
                    "crate::protocols::{}::{}::{}",
                    protocol.name,
                    interface.name,
                    snake_to_pascal(&interface.name)
                ))
                .expect("Failed to parse full interface path");

                let module_path: syn::Path = syn::parse_str(&format!(
                    "crate::protocols::{}::{}",
                    protocol.name, interface.name,
                ))
                .expect("Failed to parse interface module path");

                (struct_path, module_path)
            };

            vec![
                quote! { use #interface_struct_path; },
                quote! { use #interface_module_path; },
            ]
        })
        .collect()
}

fn generate_extensions_for_protocol(protocol: &WaylandProtocol) -> TokenStream {
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .flat_map(generate_extension_for_interface)
        .collect()
}

fn generate_extension_for_interface(interface: &WaylandInterface) -> Option<TokenStream> {
    let event_extensions: Vec<TokenStream> = interface
        .items
        .iter()
        .filter_map(|item| match item {
            InterfaceItem::Event(event) => {
                Some(generate_extension_method_for_event(event, &interface.name))
            }
            _ => None,
        })
        .collect();

    let interface_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let interface_name_ext = format_ident!(
        "{}",
        format_wayland_interface_to_rust_extension_struct(&interface.name)
    );
    Some(quote! {
        pub struct #interface_name_ext {
            pub wrapped: #interface_name
        }

        impl #interface_name_ext {
            #(#event_extensions)*
        }
    })
}

fn generate_extension_method_for_event(event: &WaylandEvent, interface_name: &str) -> TokenStream {
    let event_name: syn::Ident = format_ident!("{}", sanitize_identifier(&event.name));
    let ffi_args: Vec<TokenStream> = event
        .args
        .iter()
        .map(|arg| {
            wayland_arg_to_ffi_rust_str(arg)
                .parse::<TokenStream>()
                .expect("Failed to parse argument as TokenStream")
        })
        .collect();

    let rust_args: Vec<TokenStream> = event
        .args
        .iter()
        .map(|arg| {
            wayland_arg_to_rust_param_str(arg, interface_name)
                .parse::<TokenStream>()
                .expect("Failed to parse argument as TokenStream")
        })
        .collect();

    // Generate the ffi code that will call into the underlying method that is defined on the object
    // The first item is the definition, and the second is the method declaration for the trait.
    quote! {
        pub fn #event_name(&mut self, #(#ffi_args),*) {
            self.wrapped.#event_name(#(#rust_args),*)
        }
    }
}

pub fn wayland_arg_to_ffi_rust_str(arg: &WaylandArg) -> String {
    let name = sanitize_identifier(&arg.name);
    let mut arg_str = match arg.type_ {
        WaylandArgType::Int => format!("{}: {}", name, "i32"),
        WaylandArgType::Uint => format!("{}: {}", name, "u32"),
        WaylandArgType::Fixed => format!("{}: {}", name, "f64"),
        WaylandArgType::String => format!("{}: {}", name, "&CxxString"),
        WaylandArgType::Object | WaylandArgType::NewId => format!(
            "{}: &Box<{}>",
            name,
            format_wayland_interface_to_rust_extension_struct(
                arg.interface.as_ref().expect("Object is missing interface")
            )
        ),
        WaylandArgType::Array => format!("{}: {}", name, "&CxxVector<u8>"),
        WaylandArgType::Fd => format!("{}: {}", name, "i32"),
    };

    if arg.allow_null.unwrap_or(false) {
        arg_str.push_str(&format!(", has_{}: bool", arg.name));
    }

    arg_str
}

fn wayland_arg_to_rust_param_str(arg: &WaylandArg, interface_name: &str) -> String {
    let name = sanitize_identifier(&arg.name);
    let mut param = match arg.type_ {
        WaylandArgType::Int | WaylandArgType::Uint | WaylandArgType::Fixed => name,
        WaylandArgType::String => format!("{}.to_string()", name),
        WaylandArgType::Object | WaylandArgType::NewId => format!("&{}.wrapped", name),
        WaylandArgType::Array => format!("{}.iter().cloned().collect()", name),
        WaylandArgType::Fd => format!("unsafe {{ BorrowedFd::borrow_raw({}) }}", name),
    };

    if let Some(e) = &arg.enum_ {
        let rust_enum_path = if let Some(dot_pos) = e.find('.') {
            let interface = &e[..dot_pos];
            let enum_name = &e[dot_pos + 1..];
            format!("{}::{}", interface, snake_to_pascal(enum_name))
        } else {
            format!("{}::{}", interface_name, snake_to_pascal(e))
        };
        param = format!(
            "{}::try_from({}).unwrap_or({}::try_from(0).unwrap())",
            rust_enum_path, param, rust_enum_path
        );
    }

    if arg.allow_null.unwrap_or(false) {
        param = format!("if has_{} {{ Some({}) }} else {{ None }}", arg.name, param);
    }

    param
}
