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
use crate::protocol_parser::{
    InterfaceItem, WaylandArg, WaylandArgType, WaylandEvent, WaylandInterface, WaylandProtocol,
};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};
use std::collections::HashMap;

/// Maps (interface_name, enum_name) to a value that is guaranteed to convert
/// into the wayland-rs enum: 0 when valid (bitfields accept empty flags, and
/// many enums have a 0-valued entry), otherwise the enum's first entry.
type EnumFallbacks = HashMap<(String, String), u32>;

fn build_enum_fallback_map(protocols: &[WaylandProtocol]) -> EnumFallbacks {
    let mut map = EnumFallbacks::new();
    for protocol in protocols {
        for interface in &protocol.interfaces {
            for item in &interface.items {
                if let InterfaceItem::Enum(e) = item {
                    let zero_is_valid = e.bitfield.unwrap_or(false)
                        || e.entries.iter().any(|entry| entry.value == 0);
                    let fallback = if zero_is_valid {
                        0
                    } else {
                        e.entries
                            .first()
                            .map(|entry| entry.value as u32)
                            .unwrap_or(0)
                    };
                    map.insert((interface.name.clone(), e.name.clone()), fallback);
                }
            }
        }
    }
    map
}

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
    let enum_fallbacks = build_enum_fallback_map(protocols);
    let extensions = protocols
        .iter()
        .flat_map(|protocol| generate_extensions_for_protocol(protocol, &enum_fallbacks));

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

fn generate_extensions_for_protocol(
    protocol: &WaylandProtocol,
    enum_fallbacks: &EnumFallbacks,
) -> TokenStream {
    protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .flat_map(|interface| generate_extension_for_interface(interface, enum_fallbacks))
        .collect()
}

fn generate_extension_for_interface(
    interface: &WaylandInterface,
    enum_fallbacks: &EnumFallbacks,
) -> Option<TokenStream> {
    let event_extensions: Vec<TokenStream> = interface
        .items
        .iter()
        .filter_map(|item| match item {
            InterfaceItem::Event(event) => Some(generate_extension_method_for_event(
                event,
                &interface.name,
                enum_fallbacks,
            )),
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
            pub fn post_error(&mut self, code: u32, message: &CxxString) {
                use wayland_server::Resource;
                if self.wrapped.is_alive() {
                    self.wrapped.post_error(
                        code,
                        message.to_string()
                    );
                }
            }

            pub fn version(&self) -> u32 {
                use wayland_server::Resource;
                self.wrapped.version()
            }

            pub fn object_id(&self) -> u32 {
                use wayland_server::Resource;
                self.wrapped.id().protocol_id()
            }

            pub fn destroy_and_delete(&self) {
                use wayland_server::Resource;
                if self.wrapped.is_alive() {
                    if let Some(handle) = self.wrapped.handle().upgrade() {
                        let _ = handle.destroy_object::<crate::wayland_server_core::ServerState>(
                            &self.wrapped.id(),
                        );
                    }
                }
            }

            #(#event_extensions)*
        }
    })
}

fn generate_extension_method_for_event(
    event: &WaylandEvent,
    interface_name: &str,
    enum_fallbacks: &EnumFallbacks,
) -> TokenStream {
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
            wayland_arg_to_rust_param_str(arg, interface_name, enum_fallbacks)
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
    let nullable = arg.allow_null.unwrap_or(false);
    let is_object = matches!(arg.type_, WaylandArgType::Object | WaylandArgType::NewId);
    let mut arg_str = match arg.type_ {
        WaylandArgType::Int => format!("{}: {}", name, "i32"),
        WaylandArgType::Uint => format!("{}: {}", name, "u32"),
        WaylandArgType::Fixed => format!("{}: {}", name, "f64"),
        WaylandArgType::String => format!("{}: {}", name, "&CxxString"),
        WaylandArgType::Object | WaylandArgType::NewId => {
            let ext = format_wayland_interface_to_rust_extension_struct(
                arg.interface.as_ref().expect("Object is missing interface"),
            );
            // A nullable object is passed as a raw pointer so that absence can be
            // represented by null. Neither `std::optional` nor `Option<&Box<_>>` can
            // cross the cxx FFI boundary, but a raw pointer can.
            if nullable {
                format!("{}: *const Box<{}>", name, ext)
            } else {
                format!("{}: &Box<{}>", name, ext)
            }
        }
        WaylandArgType::Array => format!("{}: {}", name, "&CxxVector<u8>"),
        WaylandArgType::Fd => format!("{}: {}", name, "i32"),
    };

    // Nullable non-object arguments are still ferried as a `(value, has_value)` pair,
    // since their value type can always be sent across the boundary.
    if nullable && !is_object {
        arg_str.push_str(&format!(", has_{}: bool", arg.name));
    }

    arg_str
}

fn wayland_arg_to_rust_param_str(
    arg: &WaylandArg,
    interface_name: &str,
    enum_fallbacks: &EnumFallbacks,
) -> String {
    let name = sanitize_identifier(&arg.name);
    let mut param = match arg.type_ {
        WaylandArgType::Int | WaylandArgType::Uint | WaylandArgType::Fixed => name.clone(),
        WaylandArgType::String => format!("{}.to_string()", name),
        WaylandArgType::Object | WaylandArgType::NewId => format!("&{}.wrapped", name),
        WaylandArgType::Array => format!("{}.iter().cloned().collect()", name),
        WaylandArgType::Fd => format!("unsafe {{ BorrowedFd::borrow_raw({}) }}", name),
    };

    if let Some(e) = &arg.enum_ {
        let (enum_interface, enum_name) = if let Some(dot_pos) = e.find('.') {
            (&e[..dot_pos], &e[dot_pos + 1..])
        } else {
            (interface_name, e.as_str())
        };
        let rust_enum_path = format!("{}::{}", enum_interface, snake_to_pascal(enum_name));
        // The fallback must be lazy (`unwrap_or_else`) and use a value that is
        // actually valid for this enum: not every enum has a 0-valued entry.
        let fallback = enum_fallbacks
            .get(&(enum_interface.to_string(), enum_name.to_string()))
            .unwrap_or_else(|| {
                panic!(
                    "Unknown enum '{}' referenced by argument '{}' of interface '{}'",
                    e, arg.name, interface_name
                )
            });
        param = format!(
            "{}::try_from({}).unwrap_or_else(|_| {}::try_from({}).unwrap())",
            rust_enum_path, param, rust_enum_path, fallback
        );
    }

    if arg.allow_null.unwrap_or(false) {
        param = match arg.type_ {
            // The nullable object arrives as a raw pointer; dereference it to recover the
            // `Option<&wrapped>` the underlying Wayland method expects.
            WaylandArgType::Object | WaylandArgType::NewId => {
                format!("unsafe {{ {name}.as_ref() }}.map(|__opt| &__opt.wrapped)")
            }
            _ => format!("if has_{} {{ Some({}) }} else {{ None }}", arg.name, param),
        };
    }

    param
}
