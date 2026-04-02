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
        use crate::wayland_server_core::*;
        use wayland_server::DisplayHandle;
        use crate::ffi::GlobalFactory;
        use cxx::UniquePtr;
        use std::sync::{Mutex, Arc};

        impl WaylandServer {
            pub fn register_globals(state: &ServerState, factory: Arc<Mutex<UniquePtr<GlobalFactory>>>) {
                let handle = state.handle;

                #(#registration_impls)*
            }
        }
    }
}

/// Generate the rust implementation for registering each global.
/// These globals wil be outputted to a separate "wayland_server_globals.rs"
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
            handle.create_global::<ServerState, #namespace_name::#interface_name::#interface_struct_name, Arc<Mutex<UniquePtr<GlobalFactory>>>>(#version, factory.clone());
        })
    }).collect()
}
