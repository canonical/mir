//! # Wayland Server Generation
//!
//! This module provides a generator for wayland_server_generated.rs.

use crate::helpers::{generate_namespace, snake_to_pascal};

use super::WaylandProtocol;
use proc_macro2::TokenStream;
use quote::{format_ident, quote};

/// Create the Rust code for the generated part of the WaylandServer struct.
/// At the moment this includes:
///
/// - The global registration method.
pub fn generate_wayland_server_generated_rs(protocols: &Vec<WaylandProtocol>) -> TokenStream {
    let registration_impls = protocols.iter().flat_map(generate_register_globals_impl);

    quote! {
        use crate::ffi::GlobalFactory;
        use cxx::UniquePtr;
        use std::sync::{Mutex, Arc};

        // Rust cannot know that my GlobalFactory is thread-safe, despite being
        // wrapped in an Arc<Mutex<...>>. As a result, we need to implement Send + Sync
        // for it.
        unsafe impl Send for GlobalFactory {}
        unsafe impl Sync for GlobalFactory {}

        impl WaylandServer {
            pub fn register_globals(state: &ServerState, factory: Arc<Mutex<UniquePtr<GlobalFactory>>>) {
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
    protocol.interfaces.iter().filter_map(|interface| {
        if !interface.is_global {
            return None;
        }

        if interface.name == "wl_display" {
            return None;
        }

        let interface_name = format_ident!("{}", interface.name);
        let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
        let version = interface.version;
        Some(quote! {
            state.handle.create_global::<ServerState, #namespace_name::#interface_name::#interface_struct_name, Arc<Mutex<UniquePtr<GlobalFactory>>>>(#version, factory.clone());
        })
    }).collect()
}
