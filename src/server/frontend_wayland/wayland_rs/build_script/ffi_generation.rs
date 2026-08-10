//! # Protocol request code generation
//!
//! This module provides a method for generating the event handlers
//! for each Wayland interface in Rust. Each event will have an FFI
//! binding as well as a full Rust implementation that will ferry the
//! data over to the running Wayland loop and execute the event.
//!
//! These requests are sent from C++.

use crate::cpp_builder::{sanitize_identifier, CppBuilder};
use crate::helpers::{
    dash_to_snake, format_wayland_interface_to_cpp_class,
    format_wayland_interface_to_rust_extension_struct,
};

use super::protocol_middleware_generation::wayland_arg_to_ffi_rust_str;
use crate::protocol_parser::{
    InterfaceItem, WaylandArgType, WaylandEvent, WaylandInterface, WaylandProtocol,
};
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

    let server_side_factory_decls = generate_server_side_factory_ffi_decls(protocols);

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
        use crate::dispatch::*;

        #[cxx::bridge(namespace = "mir::wayland_rs")]
        #[allow(dead_code, unused_imports)]
        mod ffi {
            extern "Rust" {
                type WaylandServer;
                fn create_wayland_server() -> Box<WaylandServer>;
                fn run(self: &mut WaylandServer, socket: &str, factory: UniquePtr<GlobalFactory>, notification_handler: UniquePtr<WaylandServerNotificationHandler>, work_callback: UniquePtr<WorkCallback>) -> Result<()>;
                fn stop(self: &WaylandServer);
                fn drain_queue(self: &mut WaylandServer) -> bool;
                fn register_fd_ready_listener(self: &WaylandServer, fd: i32, callback: UniquePtr<FdReadyCallback>);
                fn create_output_global(self: &WaylandServer, binder: UniquePtr<OutputGlobalBinder>) -> Box<OutputGlobal>;

                type OutputGlobal;

                type WaylandClient;
                fn pid(self: &WaylandClient) -> Result<i32>;
                fn uid(self: &WaylandClient) -> Result<u32>;
                fn gid(self: &WaylandClient) -> Result<u32>;
                fn id(self: &WaylandClient) -> Box<WaylandClientId>;
                fn clone_box(self: &WaylandClient) -> Box<WaylandClient>;
                fn socket_fd(self: &WaylandClient) -> Result<i32>;
                fn name(self: &WaylandClient) -> Result<String>;
                fn kill(self: &WaylandClient, object_id: u32, code: u32, message: &CxxString);

                type WaylandClientId;
                fn equals(self: &WaylandClientId, id: &Box<WaylandClientId>) -> bool;

                #(#rust_types)*
                #(#rust_tokens)*
                #(#server_side_factory_decls)*
            }

            unsafe extern "C++" {
                #(#cpp_tokens)*
            }
        }
    }
}

/// Generate the `extern "Rust"` bridge declarations for the server-side
/// object-creation helpers produced in `mod dispatch`.
///
/// For every interface used as the `new_id` of an event we expose:
/// * `allocate_<interface>(client, version) -> Box<Middleware>` and
/// * `set_<interface>_inner(instance, object)`.
fn generate_server_side_factory_ffi_decls(protocols: &Vec<WaylandProtocol>) -> Vec<TokenStream> {
    let mut seen: Vec<String> = Vec::new();
    let mut decls: Vec<TokenStream> = Vec::new();

    for protocol in protocols {
        for interface in &protocol.interfaces {
            for item in &interface.items {
                let InterfaceItem::Event(event) = item else {
                    continue;
                };
                for arg in &event.args {
                    if arg.type_ != WaylandArgType::NewId {
                        continue;
                    }
                    let Some(child_name) = arg.interface.as_ref() else {
                        continue;
                    };
                    if child_name == "wl_display" || child_name == "wl_registry" {
                        continue;
                    }
                    if seen.iter().any(|s| s == child_name) {
                        continue;
                    }
                    seen.push(child_name.clone());

                    let create_fn = format_ident!("allocate_{}", dash_to_snake(child_name));
                    let set_inner_fn = format_ident!("set_{}_inner", dash_to_snake(child_name));
                    let middleware_struct = format_ident!(
                        "{}",
                        format_wayland_interface_to_rust_extension_struct(child_name)
                    );
                    let cpp_struct =
                        format_ident!("{}", format_wayland_interface_to_cpp_class(child_name));

                    decls.push(quote! {
                        fn #create_fn(client: &WaylandClient, version: u32) -> Result<Box<#middleware_struct>>;
                        fn #set_inner_fn(instance: &#middleware_struct, object: SharedPtr<#cpp_struct>);
                    });
                }
            }
        }
    }

    decls
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
        fn version(self: &#interface_name_ext) -> u32;
        fn object_id(self: &#interface_name_ext) -> u32;
        fn destroy_and_delete(self: &#interface_name_ext);
        #(#events)*
    }
}

/// An event carries a raw pointer across the FFI boundary (and therefore must be declared
/// `unsafe fn`) when it has a nullable object argument.
fn event_uses_raw_pointer(event: &WaylandEvent) -> bool {
    event.args.iter().any(|arg| {
        arg.allow_null.unwrap_or(false)
            && matches!(arg.type_, WaylandArgType::Object | WaylandArgType::NewId)
    })
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

    if event_uses_raw_pointer(event) {
        quote! {
            unsafe fn #event_name(self: &mut #interface_name, #(#args),*);
        }
    } else {
        quote! {
            fn #event_name(self: &mut #interface_name, #(#args),*);
        }
    }
}
