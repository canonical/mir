//! # Wayland Server Generation
//!
//! This module provides a generator for wayland_server_generated.rs.

use crate::helpers::{generate_namespace, natively_implemented_protocols, snake_to_pascal};

use super::WaylandProtocol;
use proc_macro2::TokenStream;
use quote::{format_ident, quote};

/// Create the Rust code for the generated part of the WaylandServer struct.
/// At the moment this includes:
///
/// - The global registration method.
/// - The names of every interface compiled into Mir (`BUILTIN_INTERFACE_NAMES`),
///   which protocols registered at runtime must not redefine.
pub fn generate_wayland_server_generated_rs(protocols: &[WaylandProtocol]) -> TokenStream {
    let registration_impls = protocols.iter().flat_map(generate_register_globals_impl);
    let builtin_interface_names = protocols
        .iter()
        .flat_map(|protocol| protocol.interfaces.iter().map(|interface| &interface.name));

    quote! {
        use crate::ffi::GlobalFactory;
        use cxx::UniquePtr;
        use std::sync::{Mutex, Arc};

        /// The names of every interface whose protocol is compiled into Mir.
        pub(crate) const BUILTIN_INTERFACE_NAMES: &[&str] = &[#(#builtin_interface_names),*];

        // Rust cannot know that my GlobalFactory is thread-safe, despite being
        // wrapped in an Arc<Mutex<...>>. As a result, we need to implement Send + Sync
        // for it.
        unsafe impl Send for GlobalFactory {}
        unsafe impl Sync for GlobalFactory {}

        impl WaylandServer {
            /// Register a global for each in-tree interface. A null `factory`
            /// registers none (a server with only runtime-loaded globals).
            pub fn register_globals(state: &ServerState, factory: Arc<Mutex<UniquePtr<GlobalFactory>>>) {
                if factory.lock().expect("No recovery from lock poisoning").is_null() {
                    return;
                }
                #(#registration_impls)*
            }
        }
    }
}

/// Generate the Rust implementation for registering each global.
/// These registrations are output to the generated "wayland_server_generated.rs"
/// file.
fn generate_register_globals_impl(protocol: &WaylandProtocol) -> Vec<TokenStream> {
    let namespace_name = generate_namespace(protocol);
    protocol
        .interfaces
        .iter()
        .filter_map(|interface| {
            if !interface.is_global {
                return None;
            }

            if natively_implemented_protocols()
                .iter()
                .cloned()
                .chain(
                    // wl_display is handled specially in wayland_server crate via the 'Display' struct.
                    std::iter::once("wl_display"),
                )
                .any(|protocol| interface.name == protocol)
            {
                return None;
            }

            // `wl_output` is advertised dynamically (one global per monitor) via
            // `WaylandServer::create_output_global`, so it must not be registered
            // statically here. See `generate_output_dynamic_global`.
            if interface.name == "wl_output" {
                return None;
            }

            let interface_name = format_ident!("{}", interface.name);
            let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
            let version = interface.version;

            Some(quote! {
                state
                    .handle
                    .create_global::<
                        ServerState,
                        #namespace_name::#interface_name::#interface_struct_name,
                        Arc<Mutex<UniquePtr<GlobalFactory>>>,
                    >(#version, factory.clone());
            })
        })
        .collect()
}
