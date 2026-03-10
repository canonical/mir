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
        use crate::wayland_server::*;
        use wayland_server::protocol::*;
        use cxx::{CxxString, CxxVector, SharedPtr};

        #(#use_imports)*

        #(#extensions)*
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
            InterfaceItem::Event(event) => Some(generate_extension_method_for_event(interface, event)),
            _ => None,
        })
        .collect();

    let interface_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let interface_name_ext = format_ident!("{}Ext", snake_to_pascal(&interface.name));
    Some(quote! {
        pub struct #interface_name_ext {
            wrapped: #interface_name
        }

        impl #interface_name_ext {
            #(#event_extensions)*
        }
    })
}

fn generate_extension_method_for_event(
    interface: &WaylandInterface,
    event: &WaylandEvent,
) -> TokenStream {
    let event_name: syn::Ident = format_ident!("{}", event.name);
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
            wayland_arg_to_rust_param_str(arg)
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

fn wayland_arg_to_ffi_rust_str(arg: &WaylandArg) -> String {
    let mut arg_str = match arg.type_ {
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
        WaylandArgType::Array => format!("{}: {}", arg.name, "&CxxVector<u8>"),
        WaylandArgType::Fd => format!("{}: {}", arg.name, "i32"),
    };

    if arg.allow_null.unwrap_or(false) {
        arg_str += format!(", has_{}: bool", arg.name).as_str();
    }

    arg_str
}

fn wayland_arg_to_rust_param_str(arg: &WaylandArg) -> String {
    let mut param = match arg.type_ {
        WaylandArgType::Int => arg.name.clone(),
        WaylandArgType::Uint => arg.name.clone(),
        WaylandArgType::Fixed => arg.name.clone(),
        WaylandArgType::String => format!("{}.to_string()", arg.name),
        WaylandArgType::Object => arg.name.clone(),
        WaylandArgType::NewId => arg.name.clone(),
        WaylandArgType::Array => format!("{}.iter().cloned().collect()", arg.name),
        WaylandArgType::Fd => arg.name.clone(),
    };

    if arg.allow_null.unwrap_or(false) {
        param = format!(
            "if has_{} {{ Some({}) }} else {{ None }}",
            arg.name, param
        );
    }

    return param;
}
