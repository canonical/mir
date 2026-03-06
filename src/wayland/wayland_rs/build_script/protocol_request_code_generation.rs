//! # Protocol request code generation
//!
//! This module provides a method for generating the event handlers
//! for each Wayland interface in Rust. Each event will have an FFI
//! binding as well as a full Rust implementation that will ferry the
//! data over to the running Wayland loop and execute the event.
//!
//! These requests are sent from C++.

use super::helpers::{snake_to_pascal, write_generated_rust_file};
use super::{
    InterfaceItem, WaylandArg, WaylandArgType, WaylandEvent, WaylandInterface, WaylandProtocol,
};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};

// Writes the Rust implementation for each protocol.
// This includes the event glue code (i.e. send_x events)
// as well as the code that will ferry that data onto the
// Wayland thread.
pub fn write_protocol_event_code(protocols: &Vec<WaylandProtocol>) {
    // First, we need to generate the binding code on ffi_rust.rs.
    let event_ffi_tokens = protocols.iter().flat_map(generate_ffi_for_protocol);

    let generated_protocol_rs = quote! {
        use crate::wayland_server::*;
        use wayland_server::protocol::*;
        use cxx::{CxxString, CxxVector, SharedPtr};

        #[cxx::bridge(namespace = "mir::wayland_rs")]
        mod ffi_rust {
            extern "Rust" {
                type WaylandServer;
                fn create_wayland_server() -> Box<WaylandServer>;
                fn run(self: &mut WaylandServer, socket: &str) -> Result<()>;
                fn stop(self: &mut WaylandServer);

                #(#event_ffi_tokens)*
            }
        }
    };

    write_generated_rust_file(generated_protocol_rs, "ffi_rust.rs");
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
        WaylandArgType::String => format!("{}: {}", arg.name, "&CxxString"),
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
        WaylandArgType::Array => format!("{}: {}", arg.name, "i32"),
        WaylandArgType::Fd => format!("{}: {}", arg.name, "i32"),
        _ => panic!("Unhandled argument type: {:?}", arg.type_),
    }
}
