//! # Protocol request code generation
//!
//! This module provides a method for generating the event handlers
//! for each Wayland interface in Rust. Each event will have an FFI
//! binding as well as a full Rust implementation that will ferry the
//! data over to the running Wayland loop and execute the event.
//!
//! These requests are sent from C++.

use crate::cpp_builder::CppBuilder;

use super::helpers::{snake_to_pascal};
use super::{
    InterfaceItem, WaylandArg, WaylandArgType, WaylandEvent, WaylandInterface, WaylandProtocol,
};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};

// Writes the ffi code.
//
// This includes Rust -> C++ with the help of the provided builders
// and C++ -> Rust.
pub fn generate_ffi(protocols: &Vec<WaylandProtocol>, builders: &Vec<CppBuilder>) -> TokenStream {
    // First, generate the imports.
    let use_imports: Vec<TokenStream> = protocols
        .iter()
        .flat_map(generate_use_imports_for_protocol)
        .collect();

    // Next, generate the C++ -> Rust side.
    //
    // Note that there is a complication here. Certain arguments on events will not
    // be able to be ferried directly from C++ to Rust. For example, C++ can only
    // call into Rust with C++ strings. Hence, while generating the code, some methods
    // will generate "middleware" functions that will ferry the data to the interface.
    let rust_tokens = protocols.iter().flat_map(generate_ffi_for_protocol);

    // Next, generate the Rust -> C++ side.
    let cpp_tokens: Vec<TokenStream> = builders
        .into_iter()
        .flat_map(|builder: &CppBuilder| builder.to_rust_cpp_bindings())
        .collect();

    // Finally, build the file.
    quote! {
        use crate::wayland_server::*;
        use wayland_server::protocol::*;
        use cxx::{CxxString, CxxVector, SharedPtr};

        #(#use_imports)*

        #[cxx::bridge(namespace = "mir::wayland_rs")]
        mod ffi {
            extern "Rust" {
                type WaylandServer;
                fn create_wayland_server() -> Box<WaylandServer>;
                fn run(self: &mut WaylandServer, socket: &str) -> Result<()>;
                fn stop(self: &mut WaylandServer);

                #(#rust_tokens)*
            }

            unsafe extern "C++" {
                #(#cpp_tokens)*
            }
        }
    }
}

fn generate_use_imports_for_protocol(protocol: &WaylandProtocol) -> Vec<TokenStream> {
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .map(|interface| {
            let full_path: syn::Path = syn::parse_str(&format!(
                "crate::protocols::{}::{}::{}",
                protocol.name,
                interface.name,
                snake_to_pascal(&interface.name)
            ))
            .expect("Failed to parse full interface path");
            quote! { use #full_path; }
        })
        .collect()
}

fn generate_ffi_for_protocol(protocol: &WaylandProtocol) -> TokenStream {
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .flat_map(generate_ffi_for_interface)
        .collect()
}

fn generate_ffi_for_interface(interface: &WaylandInterface) -> TokenStream {
    let events: Vec<TokenStream>  = interface
        .items
        .iter()
        .filter_map(|item| match item {
            InterfaceItem::Event(request) => Some(generate_ffi_for_event(interface, request)),
            _ => None,
        })
        .collect();

    let interface_name = format_ident!("{}", snake_to_pascal(&interface.name));
    quote! {
        type #interface_name;
        #(#events)*
    }
}

fn generate_ffi_for_event(interface: &WaylandInterface, event: &WaylandEvent) -> TokenStream {
    let interface_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let event_name: syn::Ident = format_ident!("{}", event.name);
    let args: Vec<TokenStream> = event
        .args
        .iter()
        .map(|arg| {
            wayland_arg_to_rust_str(arg)
                .parse::<TokenStream>()
                .expect("Failed to parse argument as TokenStream")
        })
        .collect();

    quote! {
        fn #event_name(self: &mut #interface_name, #(#args),*);
    }
}

fn wayland_arg_to_rust_str(arg: &WaylandArg) -> String {
    match arg.type_ {
        WaylandArgType::Int => format!("{}: {}", arg.name, "i32"),
        WaylandArgType::Uint => format!("{}: {}", arg.name, "u32"),
        WaylandArgType::Fixed => format!("{}: {}", arg.name, "f64"),
        WaylandArgType::String => format!("{}: {}", arg.name, "CxxString"),
        WaylandArgType::Object => format!(
            "{}: &Box<{}>",
            arg.name,
            snake_to_pascal(
                arg.interface
                    .clone()
                    .expect("Object is missing interface")
                    .as_str(),
            )
        ),
        WaylandArgType::NewId => format!("{}: {}", arg.name, "i32"),
        WaylandArgType::Array => format!("{}: {}", arg.name, "Vec<u8>"),
        WaylandArgType::Fd => format!("{}: {}", arg.name, "i32")
    }
}
