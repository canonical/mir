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

use crate::cpp_builder::sanitize_identifier;
use crate::helpers::{
    dash_to_snake_ident, format_has_arg_ident, format_wayland_interface_to_cpp_class,
    format_wayland_interface_to_rust_extension_struct, generate_namespace, snake_to_pascal,
};
use crate::protocol_parser::{
    InterfaceItem, WaylandArg, WaylandArgType, WaylandInterface, WaylandProtocol, WaylandRequest,
};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};
use syn::Ident;

pub fn generate_dispatch_rs(protocols: &Vec<WaylandProtocol>) -> TokenStream {
    let generated_dispatch_implementations = protocols
        .iter()
        .map(|protocol| generate_dispatch_implementations(protocol));

    quote! {
        #[allow(dead_code, unused_imports)]
        mod dispatch {
            use wayland_server::{Client, DataInit, Dispatch, GlobalDispatch, New, DisplayHandle, Resource};
            use wayland_server::backend::{ClientId, ObjectId};
            use crate::protocols;
            use crate::wayland_server_core::ServerState;
            use crate::ffi;
            use crate::wayland_client::{WaylandClient, WaylandClientId};
            use std::os::fd::{AsRawFd, RawFd};
            use std::sync::{Arc, LazyLock, Mutex, RwLock};
            use std::collections::HashMap;
            use std::ffi::CString;

            static OBJECT_REGISTRY: LazyLock<RwLock<HashMap<u32, ObjectId>>> = LazyLock::new(|| RwLock::new(HashMap::new()));

            fn register_resource(resource: &impl Resource) {
                let id = resource.id();
                OBJECT_REGISTRY.write().unwrap_or_else(|e| e.into_inner()).insert(id.protocol_id(), id);
            }

            fn unregister_resource(resource: &impl Resource) {
                OBJECT_REGISTRY.write().unwrap_or_else(|e| e.into_inner()).remove(&resource.id().protocol_id());
            }

            fn parse_post_error(what: &str) -> (u32, u32, String) {
                let mut parts = what.splitn(3, ':');
                let object_id = parts.next()
                    .and_then(|s| s.trim().parse::<u32>().ok())
                    .unwrap_or(0);
                let code = parts.next()
                    .and_then(|s| s.trim().parse::<u32>().ok())
                    .unwrap_or(0);
                let message = parts.next()
                    .map(|s| s.trim().to_string())
                    .unwrap_or_else(|| what.to_string());
                (object_id, code, message)
            }

            fn post_protocol_error(resource: &impl Resource, object_id: u32, code: u32, message: String) {
                let target_id = OBJECT_REGISTRY.read().unwrap_or_else(|e| e.into_inner()).get(&object_id).cloned();
                if let Some(handle) = resource.handle().upgrade() {
                    let id = target_id.unwrap_or_else(|| resource.id());
                    handle.post_error(id, code, CString::new(message).expect("Error message contained null byte"));
                }
            }

            #(#generated_dispatch_implementations)*
        }
    }
}

/// Generate a GlobalDispatch implementation for a single interface.
fn generate_global_dispatch_impl(
    interface: &WaylandInterface,
    namespace_name: &TokenStream,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name);

    if interface_name == "wl_display" {
        // wl_display is handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let ext_interface_struct_name = format_ident!(
        "{}",
        format_wayland_interface_to_rust_extension_struct(&interface.name)
    );
    let wrapper_struct_name = format_ident!("{}Wrapper", snake_to_pascal(&interface.name));
    let create_global_method = format_ident!("create_{}", &interface.name);
    let interface_name_str = &interface.name;
    quote! {
        impl GlobalDispatch<#namespace_name::#interface_name::#interface_struct_name, Arc<Mutex<cxx::UniquePtr<ffi::GlobalFactory>>>>
            for ServerState
        {
            fn bind(
                _state: &mut Self,
                handle: &wayland_server::DisplayHandle,
                client: &wayland_server::Client,
                resource: New<#namespace_name::#interface_name::#interface_struct_name>,
                // The global data is an Arc<Mutex<...>> instead of just a UniquePtr because it
                // has to be accessed mutability in order to call methods across the Rust -> C++
                // boundary.
                global_data: &Arc<Mutex<cxx::UniquePtr<ffi::GlobalFactory>>>,
                data_init: &mut wayland_server::DataInit<'_, Self>,
            ) {
                use crate::ffi;

                // Step 1: Create a wrapper with no inner value and register it via data_init.
                let wrapper = Arc::new(Mutex::new(#wrapper_struct_name { inner: None }));
                let instance = data_init.init(resource, wrapper.clone());
                register_resource(&instance);
                let protocol_id = Resource::id(&instance).protocol_id();

                // Step 2: Create the middleware object wrapping the wayland resource.
                let boxed = Box::new(crate::middleware::#ext_interface_struct_name{ wrapped: instance });

                // Step 3: Call the C++ factory with client, middleware, and object_id
                // so the C++ object is fully initialized from the start.
                let wayland_client = Box::new(WaylandClient::new(client.clone(), handle.clone()));
                let mut guard = global_data.lock().unwrap();
                let global = (&mut *guard).pin_mut().#create_global_method(wayland_client, boxed, protocol_id);

                // Step 4: Store the fully-initialized C++ object in the wrapper.
                wrapper.lock().unwrap().inner = Some(global);
            }

            fn can_view(client: Client, global_data: &Arc<Mutex<cxx::UniquePtr<ffi::GlobalFactory>>>) -> bool {
                let interface_name = #interface_name_str;
                let client_id = Box::new(WaylandClientId::new(client.id()));
                let mut guard = global_data.lock().unwrap();
                (&mut *guard).pin_mut().can_view(interface_name, client_id)
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
            WaylandArgType::Uint => Some(quote! {
                let #arg_name = u32::from(#arg_name);
            }),
            _ => None,
        };
    }

    match arg.type_ {
        // The Wayland Rust crate will provide us with raw Wayland Rust objects.
        // C++ expects the shared_ptr data that backs these rust objects to be sent
        // as parameters for objects. We lock the wrapper and clone the SharedPtr
        // to avoid holding a mutex guard across the FFI boundary.
        WaylandArgType::Object => {
            let arg_type = format_ident!(
                "{}",
                format_wayland_interface_to_cpp_class(
                    arg.interface.as_ref().expect("Object is missing interface"),
                )
            );
            let wrapper_type = format_ident!(
                "{}Wrapper",
                snake_to_pascal(arg.interface.as_ref().expect("Object is missing interface"))
            );
            if arg.allow_null.unwrap_or(false) {
                let has_arg_name = format_has_arg_ident(&arg.name);
                Some(quote! {
                    let #has_arg_name = #arg_name.is_some();
                    let #arg_name: cxx::SharedPtr<ffi::#arg_type> = match #arg_name.as_ref() {
                        Some(o) => {
                            let guard = o.data::<Arc<Mutex<#wrapper_type>>>().unwrap().lock().unwrap();
                            guard.inner.clone().unwrap_or_else(|| cxx::SharedPtr::null())
                        }
                        None => cxx::SharedPtr::<ffi::#arg_type>::null()
                    };
                    let #arg_name = &#arg_name;
                })
            } else {
                Some(quote! {
                    let #arg_name: cxx::SharedPtr<ffi::#arg_type> = {
                        let guard = #arg_name.data::<Arc<Mutex<#wrapper_type>>>().unwrap().lock().unwrap();
                        guard.inner.clone().unwrap_or_else(|| cxx::SharedPtr::null())
                    };
                    let #arg_name = &#arg_name;
                })
            }
        }
        WaylandArgType::Fd => Some(quote! {
            let #arg_name = #arg_name.as_raw_fd();
        }),
        WaylandArgType::String if arg.allow_null.unwrap_or(false) => {
            let has_arg_name = format_has_arg_ident(&arg.name);
            Some(quote! {
                let #has_arg_name = #arg_name.is_some();
                let #arg_name = match #arg_name {
                    Some(o) => o,
                    None => String::new()
                };
            })
        }
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

    let arg_to_tokens = |arg: &WaylandArg| {
        let arg_name = format_ident!("{}", arg.name.as_str());
        if arg.allow_null.unwrap_or(false)
            && matches!(arg.type_, WaylandArgType::Object | WaylandArgType::String)
        {
            let has_arg_name = format_has_arg_ident(&arg.name);
            vec![quote! { #arg_name }, quote! { #has_arg_name }]
        } else {
            vec![quote! { #arg_name }]
        }
    };

    if let Some(new_id_arg) = new_id_arg {
        let new_id_name = format_ident!("{}", new_id_arg.name);
        let ext_interface_struct_name = format_ident!(
            "{}",
            format_wayland_interface_to_rust_extension_struct(
                new_id_arg
                    .interface
                    .as_ref()
                    .expect("NewId is missing interface")
            )
        );
        let child_wrapper_struct_name = format_ident!(
            "{}Wrapper",
            snake_to_pascal(
                new_id_arg
                    .interface
                    .as_ref()
                    .expect("NewId is missing interface")
            )
        );
        let call_arg_names: Vec<TokenStream> = request
            .args
            .iter()
            .filter(|arg| arg.type_ != WaylandArgType::NewId)
            .flat_map(arg_to_tokens)
            .collect();

        quote! {
            // Step 1: Create a wrapper with no inner value and register it via data_init.
            let child_wrapper = Arc::new(Mutex::new(#child_wrapper_struct_name { inner: None }));
            let instance = data_init.init(#new_id_name, child_wrapper.clone());
            register_resource(&instance);
            let protocol_id = Resource::id(&instance).protocol_id();

            // Step 2: Create the middleware object wrapping the child resource.
            let boxed = Box::new(crate::middleware::#ext_interface_struct_name{ wrapped: instance });

            // Step 3: Call the parent's request method with the child middleware so the
            // child C++ object is fully initialized from the start.
            let mut guard = data.lock().unwrap();
            let inner = guard.inner.as_mut().expect("Request dispatched on uninitialized resource");
            // SAFETY: The mutex guard provides the only mutable access while the call is
            // in progress. The pinned reference is used only for this FFI call.
            match unsafe { inner.pin_mut_unchecked().#snake_request_name(#( #call_arg_names, )* boxed, protocol_id) } {
                Ok(child) => {
                    // Step 4: Store the fully-initialized child C++ object in the wrapper.
                    child_wrapper.lock().unwrap().inner = Some(child);
                }
                Err(err) => {
                    let (object_id, code, message) = parse_post_error(err.what());
                    post_protocol_error(resource, object_id, code, message);
                }
            }
        }
    } else {
        let call_arg_names: Vec<TokenStream> =
            request.args.iter().flat_map(arg_to_tokens).collect();

        quote! {
            let mut guard = data.lock().unwrap();
            let inner = guard.inner.as_mut().expect("Request dispatched on uninitialized resource");
            // SAFETY: The mutex guard provides the only mutable access while the call is
            // in progress. The pinned reference is used only for this FFI call.
            if let Err(err) = unsafe { inner.pin_mut_unchecked().#snake_request_name(#( #call_arg_names ),*) } {
                let (object_id, code, message) = parse_post_error(err.what());
                post_protocol_error(resource, object_id, code, message);
            }
        }
    }
}

/// Generate request handler arms for an interface's requests.
fn generate_request_handler_arms(
    interface: &WaylandInterface,
    namespace_name: &TokenStream,
    interface_name: &Ident,
) -> Vec<TokenStream> {
    interface
        .items
        .iter()
        .filter_map(|item| {
            let InterfaceItem::Request(request) = item else {
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
    interface: &WaylandInterface,
    namespace_name: &TokenStream,
    is_wayland_protocol: bool,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name);

    if interface_name == "wl_display" || interface_name == "wl_registry" {
        // wl_display and wl_registry are handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let protocol_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let ext_struct_name =
        format_ident!("{}", format_wayland_interface_to_cpp_class(&interface.name));
    let wrapper_struct_name = format_ident!("{}Wrapper", snake_to_pascal(&interface.name));

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

    let resource_name = format_ident!(
        "{}",
        if interface_has_requests {
            "resource"
        } else {
            "_resource"
        }
    );

    quote! {
        unsafe impl Send for ffi::#ext_struct_name {}
        unsafe impl Sync for ffi::#ext_struct_name {}

        struct #wrapper_struct_name {
            inner: Option<cxx::SharedPtr<ffi::#ext_struct_name>>,
        }

        impl Dispatch<#namespace_name::#interface_name::#protocol_struct_name, Arc<Mutex<#wrapper_struct_name>>>
            for ServerState
        {
            fn request(
                _state: &mut Self,
                _client: &Client,
                #resource_name: &#namespace_name::#interface_name::#protocol_struct_name,
                request: <#namespace_name::#interface_name::#protocol_struct_name as wayland_server::Resource>::Request,
                #data_name: &Arc<Mutex<#wrapper_struct_name>>,
                _dhandle: &DisplayHandle,
                #data_init_name: &mut DataInit<'_, Self>,
            ) {
                match request {
                    #(#request_handler_arms),*
                }
            }

            fn destroyed(
                _state: &mut Self,
                _client: ClientId,
                resource: &#namespace_name::#interface_name::#protocol_struct_name,
                _data: &Arc<Mutex<#wrapper_struct_name>>,
            ) {
                unregister_resource(resource);
            }
        }
    }
}

/// Generate all dispatch implementations (both GlobalDispatch and Dispatch) for a single protocol.
fn generate_dispatch_implementations(protocol: &WaylandProtocol) -> TokenStream {
    let namespace_name = generate_namespace(protocol);

    let global_interfaces: Vec<&WaylandInterface> = protocol
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
