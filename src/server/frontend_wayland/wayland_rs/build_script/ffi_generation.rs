//! # Protocol request code generation
//!
//! This module provides a method for generating the event handlers
//! for each Wayland interface in Rust. Each event will have an FFI
//! binding as well as a full Rust implementation that will ferry the
//! data over to the running Wayland loop and execute the event.
//!
//! These requests are sent from C++.

use crate::cpp_builder::{sanitize_identifier, CppBuilder};
use crate::helpers::format_wayland_interface_to_rust_extension_struct;

use super::protocol_middleware_generation::wayland_arg_to_ffi_rust_str;
use super::{InterfaceItem, WaylandEvent, WaylandInterface, WaylandProtocol};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};

// Writes the ffi code.
//
// This includes Rust -> C++ with the help of the provided builders
// and C++ -> Rust.
pub fn generate_ffi(protocols: &Vec<WaylandProtocol>, builders: &Vec<CppBuilder>) -> TokenStream {
    // First, generate the C++ -> Rust FFI code.
    let rust_tokens = protocols.iter().flat_map(generate_ffi_for_protocol);

    let rust_types = protocols.iter().flat_map(generate_rust_types);

    // Next, generate the Rust -> C++ side.
    let cpp_tokens: Vec<TokenStream> = builders
        .into_iter()
        .flat_map(|builder: &CppBuilder| builder.to_rust_cpp_bindings())
        .collect();

    // Finally, build the file.
    quote! {
        use crate::wayland_server_core::*;
        use crate::wayland_client::*;
        use crate::middleware::*;

        #[cxx::bridge(namespace = "mir::wayland_rs")]
        #[allow(dead_code, unused_imports)]
        mod ffi {
            extern "Rust" {
                type WaylandServer;
                fn create_wayland_server() -> Box<WaylandServer>;
                fn run(self: &mut WaylandServer, socket: &str, factory: UniquePtr<GlobalFactory>, notification_handler: UniquePtr<WaylandServerNotificationHandler>) -> Result<()>;
                fn stop(self: &mut WaylandServer);

                fn next_serial() -> u32;

                type WaylandEventLoopHandle;
                fn watch_fd(self: &WaylandEventLoopHandle, fd: i32, callback: UniquePtr<FdReadyCallback>) -> Box<FdWatchToken>;
                fn spawn(self: &WaylandEventLoopHandle, work: UniquePtr<WorkCallback>);
                fn clone_box(self: &WaylandEventLoopHandle) -> Box<WaylandEventLoopHandle>;

                type FdWatchToken;
                fn cancel(self: &mut FdWatchToken);

                type WaylandClient;
                fn pid(self: &WaylandClient) -> Result<i32>;
                fn uid(self: &WaylandClient) -> Result<u32>;
                fn gid(self: &WaylandClient) -> Result<u32>;
                fn socket_fd(self: &WaylandClient) -> Result<i32>;
                fn name(self: &WaylandClient) -> Result<String>;
                fn id(self: &WaylandClient) -> Box<WaylandClientId>;
                fn clone_box(self: &WaylandClient) -> Box<WaylandClient>;
                fn kill(self: &WaylandClient, object_id: u32, code: u32, message: &CxxString);

                type WaylandClientId;
                fn equals(self: &WaylandClientId, id: &Box<WaylandClientId>) -> bool;

                #(#rust_types)*
                #(#rust_tokens)*
            }

            unsafe extern "C++" {
                #(#cpp_tokens)*
            }
        }
    }
}

fn generate_rust_types(protocol: &WaylandProtocol) -> Vec<TokenStream> {
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .map(|interface| {
            let interface_name_ext = format_ident!(
                "{}",
                format_wayland_interface_to_rust_extension_struct(&interface.name)
            );
            quote! { type #interface_name_ext; }
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
    let interface_name_ext = format_ident!(
        "{}",
        format_wayland_interface_to_rust_extension_struct(&interface.name)
    );

    let events: Vec<TokenStream> = interface
        .items
        .iter()
        .filter_map(|item| match item {
            InterfaceItem::Event(request) => Some(generate_ffi_for_event(interface, request)),
            _ => None,
        })
        .collect();

    quote! {
        fn post_error(self: &mut #interface_name_ext, code: u32, message: &CxxString);
        #(#events)*
    }
}

fn generate_ffi_for_event(interface: &WaylandInterface, event: &WaylandEvent) -> TokenStream {
    let interface_name = format_ident!(
        "{}",
        format_wayland_interface_to_rust_extension_struct(&interface.name)
    );
    let event_name: syn::Ident = format_ident!("{}", sanitize_identifier(&event.name));
    let args: Vec<TokenStream> = event
        .args
        .iter()
        .map(|arg| {
            wayland_arg_to_ffi_rust_str(arg)
                .parse::<TokenStream>()
                .expect("Failed to parse argument as TokenStream")
        })
        .collect();

    quote! {
        fn #event_name(self: &mut #interface_name, #(#args),*);
    }
}
