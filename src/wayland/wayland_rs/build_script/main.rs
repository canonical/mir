/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

mod cpp_builder;
mod protocol_parser;

use cpp_builder::{
    sanitize_identifier, CppArg, CppBuilder, CppClass, CppEnum, CppEnumOption, CppMethod,
    CppNamespace, CppType,
};
use proc_macro2::TokenStream;
use protocol_parser::{
    parse_protocols, InterfaceItem, WaylandArg, WaylandArgType, WaylandEnum, WaylandInterface,
    WaylandProtocol, WaylandRequest,
};
use quote::{format_ident, quote};
use std::{env, fs, path::Path};
use syn::Ident;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let include_path = Path::new(&manifest_dir).join("include");

    cxx_build::bridges(vec!["src/lib.rs"])
        .include(&include_path)
        .compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");

    // First, parse the protocol XML files.
    let protocols: Vec<WaylandProtocol> = parse_protocols();

    // Next, generate the protocols.rs file.
    write_protocols_rs(&protocols);

    // Next, generate the dispatch and global dispatch methods.
    write_dispatch_rs(&protocols);

    // Next, generate a C++ abstract class for each interface.
    write_cpp_protocol_headers(&protocols);
}

fn write_protocols_rs(protocols: &Vec<WaylandProtocol>) {
    let generated_protocols = protocols.iter().map(|protocol| {
        // We rely on the wayland_server crate for the core Wayland protocol.
        if protocol.name == "wayland" {
            return quote! {};
        }

        let struct_name = dash_to_snake_ident(&protocol.name);
        let path = &protocol.path;

        // Add use statements for other protocol dependencies.
        let protocol_dependencies: std::collections::HashSet<String> = protocol
            .dependencies
            .iter()
            .filter_map(|dependency| {
                protocols
                    .iter()
                    .find(|dep_protocol| {
                        dep_protocol.name != protocol.name
                            && dep_protocol
                                .interfaces
                                .iter()
                                .any(|iface| iface.name == *dependency)
                    })
                    .map(|dep_protocol| dep_protocol.name.clone())
            })
            .collect();

        let mut server_code_use_statements = quote! {
            use super::wayland_server;
            use self::interfaces::*;
        };

        for dependency in &protocol_dependencies {
            let dep_struct_name = dash_to_snake_ident(dependency);
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    server_code_use_statements = quote! {
                        #server_code_use_statements
                        use wayland_server::protocol::*;
                    };
                } else {
                    server_code_use_statements = quote! {
                        #server_code_use_statements
                        use super::#dep_struct_name::*;
                    };
                }
            }
        }

        let mut interface_code_use_statements: TokenStream = quote! {};
        for dependency in &protocol_dependencies {
            let dep_struct_name = dash_to_snake_ident(dependency);
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
                        use wayland_server::protocol::__interfaces::*;
                    };
                } else {
                    // Note: #[allow(unused_imports)] is used here to suppress warnings in case some protocols
                    // do not actually use any interfaces from their dependencies. This only happens in a
                    // input-method-unstable-v2 because it uses the enums from text-input-unstable-v3 but not
                    // the interface name or anything else.
                    //
                    // This is much simpler than piping the data through the protocol parser, so let's keep things
                    // straightforward for now.
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements

                        #[allow(unused_imports)]
                        use super::super::#dep_struct_name::interfaces::*;
                    };
                }
            }
        }

        quote! {
            pub mod #struct_name {

                #server_code_use_statements

                pub mod interfaces {
                    #interface_code_use_statements
                    wayland_scanner::generate_interfaces!(#path);
                }

                wayland_scanner::generate_server_code!(#path);
            }
        }
    });

    let generated_protocol_rs = quote! {
        use wayland_server;

        #(#generated_protocols)*
    };

    write_generated_rust_file(generated_protocol_rs, "protocols.rs");
}

/// Generate the namespace token for a protocol (either wayland_server::protocol or protocols::module_name).
fn generate_namespace(protocol: &WaylandProtocol) -> TokenStream {
    if protocol.name == "wayland" {
        quote! { wayland_server::protocol }
    } else {
        let protocol_module = dash_to_snake_ident(&protocol.name);
        quote! { protocols::#protocol_module }
    }
}

/// Generate a GlobalDispatch implementation for a single interface.
fn generate_global_dispatch_impl(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name.to_string());

    if interface_name == "wl_display" {
        // wl_display is handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let create_global_method = format_ident!("create_{}", &interface.name);
    quote! {
        impl GlobalDispatch<#namespace_name::#interface_name::#interface_struct_name, ffi_cpp::GlobalFactory>
            for ServerState
        {

            fn bind(
                _state: &mut Self,
                _handle: &wayland_server::DisplayHandle,
                _client: &wayland_server::Client,
                resource: New<#namespace_name::#interface_name::#interface_struct_name>,
                global_data: &ffi_cpp::GlobalFactory,
                data_init: &mut wayland_server::DataInit<'_, Self>,
            ) {
                use ffi_cpp;
                let global = global_data.#create_global_method();
                data_init.init(resource, global);
            }
        }
    }
}

/// Generate a TokenStream that transforms a single argument for crossing the Rust/C++ boundary.
fn transform_argument_for_cpp(arg: &WaylandArg) -> Option<TokenStream> {
    let arg_name = format_ident!("{}", arg.name);

    // Enums are provided as WlEnum, so we need to cast them to u32 so that
    // they can be sent across the barrier.
    if arg.enum_.is_some() {
        return match arg.type_ {
            // The "int" appears to be a faff on the `WlSurface.set_buffer_transform` call,
            // since most enums are u32s, so much so that the wayland Rust crate assumes
            // that this is always the case.
            WaylandArgType::Int => Some(quote! {
                let #arg_name = u32::from(#arg_name) as i32;
            }),
            _ => Some(quote! {
                let #arg_name = u32::from(#arg_name);
            }),
        };
    }

    match arg.type_ {
        // The Wayland Rust crate will provide us with raw Wayland Rust objects.
        // C++ expects the shared_ptr data that backs these rust objects to be sent
        // as parameters for objects.
        WaylandArgType::Object => {
            let arg_type = format_ident!(
                "{}",
                snake_to_pascal(arg.interface.clone().unwrap().as_str())
            );
            if arg.allow_null.unwrap_or(false) {
                let null_ptr_name = format_ident!("__{}_null_ptr", arg.name.replace('-', "_"));
                Some(quote! {
                    let #null_ptr_name = cxx::UniquePtr::<ffi_cpp::#arg_type>::null();
                    let #arg_name: &cxx::UniquePtr<ffi_cpp::#arg_type> = match #arg_name.as_ref() {
                        Some(o) => o.data().unwrap(),
                        None => &#null_ptr_name
                    };
                })
            } else {
                Some(quote! {
                    let #arg_name: &cxx::UniquePtr<ffi_cpp::#arg_type> = #arg_name.data().unwrap();
                })
            }
        }
        WaylandArgType::Fd => Some(quote! {
            let #arg_name = #arg_name.as_raw_fd();
        }),
        WaylandArgType::String if arg.allow_null.unwrap_or(false) => Some(quote! {
            let #arg_name = match #arg_name {
                Some(o) => o,
                None => String::new()
            };
        }),
        _ => None,
    }
}

/// Generate the body of a request handler arm.
///
/// Requests come in two distinct forms:
/// 1. Regular requests that ferry their data directly into the C++ method.
/// 2. Resource creation requests which ask the C++ interface to create
///    the resource for them. These methods MUST call data_init.init()
///    with the newly created C++ resource.
fn generate_request_body(request: &WaylandRequest) -> TokenStream {
    let snake_request_name = dash_to_snake_ident(&sanitize_identifier(request.name.as_str()));
    let new_id_arg = request
        .args
        .iter()
        .find(|arg| arg.type_ == WaylandArgType::NewId);

    if let Some(new_id_arg) = new_id_arg {
        let new_id_name = format_ident!("{}", new_id_arg.name);
        let call_arg_names: Vec<TokenStream> = request
            .args
            .iter()
            .filter(|arg| arg.type_ != WaylandArgType::NewId)
            .map(|arg| {
                let arg_name = format_ident!("{}", arg.name.as_str());
                quote! { #arg_name }
            })
            .collect();

        quote! {
            data_init.init(#new_id_name, data.#snake_request_name(#( #call_arg_names ),*));
        }
    } else {
        let call_arg_names: Vec<TokenStream> = request
            .args
            .iter()
            .map(|arg| {
                let arg_name = format_ident!("{}", arg.name.as_str());
                quote! { #arg_name }
            })
            .collect();

        quote! {
            data.#snake_request_name(#( #call_arg_names ),*);
        }
    }
}

/// Generate request handler arms for an interface's requests.
fn generate_request_handler_arms(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
    interface_name: &Ident,
) -> Vec<TokenStream> {
    interface
        .items
        .iter()
        .filter_map(|item| {
            let protocol_parser::InterfaceItem::Request(request) = item else {
                return None;
            };

            let request_name = format_ident!("{}", snake_to_pascal(&request.name));
            let arg_names: Vec<TokenStream> = request
                .args
                .iter()
                .map(|arg| {
                    let arg_name = format_ident!("{}", arg.name.as_str());
                    quote! { #arg_name }
                })
                .collect();

            let transformed_args: Vec<TokenStream> = request
                .args
                .iter()
                .filter_map(transform_argument_for_cpp)
                .collect();

            let body = generate_request_body(request);

            Some(quote! {
                #namespace_name::#interface_name::Request::#request_name { #( #arg_names ),* } => {
                    #( #transformed_args )*
                    #body
                }
            })
        })
        .collect()
}

/// Generate a Dispatch implementation for a single interface.
fn generate_dispatch_impl(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
    is_wayland_protocol: bool,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name);

    if interface_name == "wl_display" || interface_name == "wl_registry" {
        // wl_display and wl_registry are handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));

    let mut request_handler_arms =
        generate_request_handler_arms(interface, namespace_name, &interface_name);

    // If the interface comes from wayland, we need to generate an empty arm because the
    // enum is marked as non-exhaustive.
    if is_wayland_protocol {
        request_handler_arms.push(quote! {
            _ => {}
        });
    }

    // This snippet checks if any request on the interface creates a new Wayland object.
    // If none do, then we can add the underscore in front of _data_init to show that it is
    // not used.
    let interface_creates_wayland_objects = interface.items.iter().any(|item| match item {
        InterfaceItem::Request(request) => request
            .args
            .iter()
            .any(|arg| arg.type_ == WaylandArgType::NewId),
        _ => false,
    });

    let data_init_name = format_ident!(
        "{}",
        if interface_creates_wayland_objects {
            "data_init"
        } else {
            "_data_init"
        }
    );

    // This snippet checks if the interface has any requests at all. If not, then data
    // will be prefixed with an underscore.
    let interface_has_requests = interface.items.iter().any(|item| match item {
        InterfaceItem::Request(_) => true,
        _ => false,
    });

    let data_name = format_ident!(
        "{}",
        if interface_has_requests {
            "data"
        } else {
            "_data"
        }
    );

    quote! {
        unsafe impl Send for ffi_cpp::#interface_struct_name {}
        unsafe impl Sync for ffi_cpp::#interface_struct_name {}

        impl Dispatch<#namespace_name::#interface_name::#interface_struct_name, cxx::UniquePtr<ffi_cpp::#interface_struct_name>>
            for ServerState
        {
            fn request(
                _state: &mut Self,
                _client: &Client,
                _resource: &#namespace_name::#interface_name::#interface_struct_name,
                request: <#namespace_name::#interface_name::#interface_struct_name as wayland_server::Resource>::Request,
                #data_name: &cxx::UniquePtr<ffi_cpp::#interface_struct_name>,
                _dhandle: &DisplayHandle,
                #data_init_name: &mut DataInit<'_, Self>,
            ) {
                match request {
                    #(#request_handler_arms),*
                }
            }
        }
    }
}

/// Generate all dispatch implementations (both GlobalDispatch and Dispatch) for a single protocol.
fn generate_dispatch_implementations(protocol: &WaylandProtocol) -> TokenStream {
    let namespace_name = generate_namespace(protocol);

    let global_interfaces: Vec<&protocol_parser::WaylandInterface> = protocol
        .interfaces
        .iter()
        .filter(|interface| interface.is_global)
        .collect();

    let global_dispatch_impls = global_interfaces
        .iter()
        .map(|interface| generate_global_dispatch_impl(interface, &namespace_name));

    let is_wayland_protocol = protocol.name == "wayland";
    let dispatch_impls = protocol
        .interfaces
        .iter()
        .map(|interface| generate_dispatch_impl(interface, &namespace_name, is_wayland_protocol));

    quote! {
        #(#global_dispatch_impls)*
        #(#dispatch_impls)*
    }
}

fn write_dispatch_rs(protocols: &Vec<WaylandProtocol>) {
    let generated_dispatch_implementations = protocols
        .iter()
        .map(|protocol| generate_dispatch_implementations(protocol));

    let generated_protocol_rs = quote! {
        use wayland_server::{Client, DataInit, Dispatch, GlobalDispatch, New, DisplayHandle, Resource};
        use crate::protocols;
        use crate::wayland_server::ServerState;
        use crate::ffi_cpp;
        use std::os::fd::{AsRawFd, RawFd};

        #(#generated_dispatch_implementations)*
    };

    write_generated_rust_file(generated_protocol_rs, "dispatch.rs");
}

fn create_global_factory(protocols: &Vec<WaylandProtocol>) -> CppBuilder {
    let mut builder: CppBuilder = CppBuilder::new(
        "MIR_WAYLANDRS_GLOBALS".to_string(),
        "global_factory".to_string(),
    );
    builder.add_include("<memory>".to_string());
    let mut namespace = CppNamespace::new(vec!["mir".to_string(), "wayland_rs".to_string()]);
    let mut class = CppClass::new("GlobalFactory".to_string());
    protocols.iter().for_each(|protocol| {
        protocol
            .interfaces
            .iter()
            .filter(|interface| interface.is_global)
            .filter(|interface| interface.name != "wl_display" && interface.name != "wl_registry")
            .for_each(|global_interface| {
                let pascal_name = snake_to_pascal(&global_interface.name);
                namespace.add_forward_declaration_class(&pascal_name);

                let method = CppMethod::new(
                    format!("create_{}", global_interface.name),
                    Some(CppType::Object(pascal_name)),
                );
                class.add_method(method);
            })
    });
    namespace.add_class(class);
    builder.add_namespace(namespace);
    builder
}

/// Write a header file for each protocol containing abstract classes per-interface.
fn write_cpp_protocol_headers(protocols: &Vec<WaylandProtocol>) {
    // First, create the global factory.
    let global_builder = create_global_factory(protocols);

    let mut builders: Vec<CppBuilder> = protocols
        .iter()
        .map(|protocol| create_cpp_builder(protocol))
        .collect();
    builders.push(global_builder);

    // Write the protocol headers
    for builder in &builders {
        write_cpp_header(&builder);
    }

    // Write the Rust FFI glue code.
    // Each builder returns a Vec of C++ type declarations (one per type),
    // intended to be placed inside an extern "C++" block, so we flatten
    // them all into a single Vec.
    let extern_blocks: Vec<TokenStream> = builders
        .into_iter()
        .flat_map(|builder: CppBuilder| builder.to_rust_bindings())
        .collect();

    let cpp_ffi_rs = quote! {
        #[cxx::bridge]
        mod ffi_cpp {
            unsafe extern "C++" {
                #(#extern_blocks)*
            }
        }
    };
    write_generated_rust_file(cpp_ffi_rs, "ffi_cpp.rs");
}

fn create_cpp_builder(protocol: &WaylandProtocol) -> CppBuilder {
    let guard = format!("MIR_WAYLANDRS_{}", protocol.name.to_uppercase());
    let mut builder = CppBuilder::new(guard, protocol.name.clone());
    let mut namespace = CppNamespace::new(vec!["mir".to_string(), "wayland_rs".to_string()]);

    let classes = protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .map(|interface| wayland_interface_to_cpp_class(interface));

    for class in classes {
        namespace.add_class(class);
    }
    for interface in &protocol.dependencies {
        namespace.add_forward_declaration_class(&snake_to_pascal(interface));
    }
    builder.add_namespace(namespace);
    builder.add_include("<rust/cxx.h>".to_string());
    builder.add_include("<memory>".to_string());

    builder
}

fn write_cpp_header(builder: &CppBuilder) {
    let filename = format!("{}.h", builder.filename);
    write_generated_cpp_file(&builder.to_cpp_header(), filename.as_str());
}

fn wayland_interface_to_cpp_class(interface: &WaylandInterface) -> CppClass {
    let mut class = CppClass::new(snake_to_pascal(&interface.name));
    let methods = interface.items.iter().filter_map(|item| {
        if let protocol_parser::InterfaceItem::Request(request) = item {
            Some(wayland_request_to_cpp_method(request))
        } else {
            None
        }
    });

    for enum_ in wayland_interface_to_enums(interface) {
        class.add_enum(enum_);
    }

    for method in methods {
        class.add_method(method);
    }

    class
}

fn wayland_interface_to_enums(interface: &WaylandInterface) -> impl Iterator<Item = CppEnum> + '_ {
    interface.items.iter().filter_map(|item| {
        if let protocol_parser::InterfaceItem::Enum(enum_) = item {
            Some(wayland_enum_to_cpp_enum(enum_))
        } else {
            None
        }
    })
}

fn wayland_enum_to_cpp_enum(enum_: &WaylandEnum) -> CppEnum {
    let mut result = CppEnum::new(snake_to_pascal(enum_.name.as_str()));

    for option in &enum_.entries {
        result.add_option(CppEnumOption {
            name: snake_to_pascal(option.name.as_str()),
            value: option.value as u32,
        });
    }

    result
}

fn wayland_request_to_cpp_method(method: &WaylandRequest) -> CppMethod {
    // Methods will have a return value in C++ if they are creating a new Wayland object.
    // Otherwsie they always return void.
    let retval = match method
        .args
        .iter()
        .find(|arg| arg.type_ == WaylandArgType::NewId)
    {
        Some(new_id_arg) => {
            let name = snake_to_pascal(new_id_arg.interface.clone().unwrap().as_str());
            Some(CppType::Object(name))
        }
        None => None,
    };

    let mut cpp_method = CppMethod::new(method.name.clone(), retval);
    let args = method
        .args
        .iter()
        .filter(|arg| arg.type_ != WaylandArgType::NewId)
        .map(|arg| {
            let type_ = match arg.type_ {
                WaylandArgType::Int => CppType::CppI32,
                WaylandArgType::Uint => CppType::CppU32,
                WaylandArgType::Fixed => CppType::CppF64,
                WaylandArgType::String => CppType::String,
                WaylandArgType::Object => CppType::Object(snake_to_pascal(
                    arg.interface
                        .clone()
                        .expect("Object is missing interface")
                        .as_str(),
                )),
                WaylandArgType::Array => CppType::Array,
                WaylandArgType::Fd => CppType::Fd,
                _ => panic!("Unhandled argument type"),
            };

            CppArg::new(type_, arg.name.clone())
        });

    for arg in args {
        cpp_method.add_arg(arg);
    }

    cpp_method
}

/// Write the generated Rust code to a file with proper formatting.
fn write_generated_rust_file(tokens: proc_macro2::TokenStream, filename: &str) {
    let out_dir = "src";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    let syntax_tree: syn::File = syn::parse2(tokens).unwrap();
    let formatted_code = prettyplease::unparse(&syntax_tree);

    fs::write(dest_path, formatted_code).unwrap();
}

/// Write the generated C++ code to the correct directory.
fn write_generated_cpp_file(content: &str, filename: &str) {
    let out_dir = "include";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    fs::create_dir_all(&out_dir).unwrap();
    fs::write(dest_path, content).unwrap();
}

fn dash_to_snake(name: &str) -> String {
    name.replace('-', "_")
}

fn dash_to_snake_ident(name: &str) -> Ident {
    format_ident!("{}", dash_to_snake(name))
}

fn snake_to_pascal(s: &str) -> String {
    s.split('_')
        .map(|word| {
            let mut chars = word.chars();
            match chars.next() {
                None => String::new(),
                Some(first) => first.to_uppercase().collect::<String>() + chars.as_str(),
            }
        })
        .collect()
}
