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
    // be able to be ferried directly from C++ to Rust. This is because the Rust
    // Wayland protocol methods take arguments that cannot be sent over the C++ -> Rust
    // boundary. To fix this, we first generate some "extensions" on structs that require
    // some middleware to ferry the data over.
    let extensions = protocols.iter().flat_map(generate_extensions_for_protocol);

    // Then we generate the C++ -> Rust FFI code.
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

        #(#extensions)*

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

fn generate_extensions_for_protocol(protocol: &WaylandProtocol) -> TokenStream {
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .flat_map(generate_extension_for_interface)
        .collect()
}

fn generate_extension_for_interface(interface: &WaylandInterface) -> Option<TokenStream> {
    let event_extensions: Vec<(TokenStream, TokenStream)>  = interface
        .items
        .iter()
        .filter_map(|item| match item {
            InterfaceItem::Event(event) => generate_extension_method_for_event(interface, event),
            _ => None,
        })
        .collect();

    if event_extensions.is_empty() {
        return None
    }

    let definitions = event_extensions.iter().map(|tuple| tuple.0.clone());
    let declarations = event_extensions.iter().map(|tuple| tuple.1.clone());
    let interface_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let interface_name_ext = format_ident!("{}Ext", snake_to_pascal(&interface.name));
    Some(quote! {
        pub trait #interface_name_ext {
            #(#declarations)*
        }

        impl #interface_name_ext for #interface_name {
            #(#definitions)*
        }
    })
}

fn generate_extension_method_for_event(interface: &WaylandInterface, event: &WaylandEvent) -> Option<(TokenStream, TokenStream)> {
    let needs_extension = event.args.iter().any(|arg| {
        arg.allow_null.unwrap_or(false) || arg.enum_.is_some() || arg.type_ == WaylandArgType::Object
            || arg.type_ == WaylandArgType::String || arg.type_ == WaylandArgType::Array
    });

    if !needs_extension {
        return None;
    }

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

    let interface_name = format_ident!("{}", snake_to_pascal(&interface.name));

    // Generate the ffi code that will call into the underlying method that is defined on the object
    // The first item is the definition, and the second is the method declaration for the trait.
    return Some((quote! {
        fn #event_name(&mut self, #(#ffi_args),*) {
            #interface_name::#event_name(self, #(#rust_args),*)
        }
    }, quote! {
        fn #event_name(&mut self, #(#ffi_args),*);
    }));
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
            wayland_arg_to_ffi_rust_str(arg)
                .parse::<TokenStream>()
                .expect("Failed to parse argument as TokenStream")
        })
        .collect();

    quote! {
        fn #event_name(self: &mut #interface_name, #(#args),*);
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
        WaylandArgType::Fd => format!("{}: {}", arg.name, "i32")
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
        WaylandArgType::Array => format!("{}.iter().collect()", arg.name),
        WaylandArgType::Fd => arg.name.clone()
    };

    if arg.allow_null.unwrap_or(false) {
        param = format!("if has_{} {{ Some({}) }} else {{ None }}", arg.name, arg.name);
    }

    return param;
}

