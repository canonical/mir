use crate::protocol_parser::{WaylandArgType, WaylandProtocol, WaylandRequest};
use proc_macro2::TokenStream;
use quote::{format_ident, quote};
use std::fs;
use syn::Ident;

/// Returns true for the libwayland core interfaces (`wl_display`, `wl_registry`).
///
/// These are managed internally by the `wayland_server` crate rather than wrapped
/// by the generator, so they have no C++ counterpart.
pub fn is_core_interface(name: &str) -> bool {
    name == "wl_display" || name == "wl_registry"
}

/// Returns true if the request takes an `object` argument referring to a libwayland
/// core interface (e.g. `wl_fixes.destroy_registry`, which takes a `wl_registry`).
///
/// Such requests cannot be forwarded to C++ because the core interface has no C++
/// wrapper; they are handled entirely on the Rust side.
pub fn request_references_core_object(request: &WaylandRequest) -> bool {
    request.args.iter().any(|arg| {
        arg.type_ == WaylandArgType::Object
            && arg
                .interface
                .as_deref()
                .map(is_core_interface)
                .unwrap_or(false)
    })
}

/// Write the generated Rust code to a file with proper formatting.
pub fn write_generated_rust_file(tokens: TokenStream, filename: &str) {
    let out_dir = "src";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    let tokens_for_error = tokens.to_string();
    let syntax_tree: syn::File = syn::parse2(tokens).unwrap_or_else(|e| {
        panic!(
            "syn::parse2 failed while generating '{filename}': {e}\n\nGenerated tokens:\n{tokens_for_error}"
        )
    });
    let formatted_code = prettyplease::unparse(&syntax_tree);

    fs::write(dest_path, formatted_code).unwrap();
}

/// Write the generated C++ code to the correct directory.
pub fn write_generated_cpp_file(content: &str, out_dir: &str, filename: &str) {
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    fs::create_dir_all(&out_dir).unwrap();
    fs::write(dest_path, content).unwrap();
}

pub fn dash_to_snake(name: &str) -> String {
    name.replace('-', "_")
}

pub fn dash_to_snake_ident(name: &str) -> Ident {
    format_ident!("{}", dash_to_snake(name))
}

pub fn snake_to_pascal(s: &str) -> String {
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

/// Strips known Wayland protocol/vendor prefixes from an interface name to
/// produce a type name matching existing C++ conventions.
///
/// Only prefixes in [`KNOWN_PREFIXES`] are stripped. Interfaces with
/// unrecognized prefixes (e.g., `xdg_`, `ext_`) keep their full name.
fn strip_wayland_interface_prefix(s: &str) -> &str {
    /// Known Wayland protocol/vendor prefixes and how many characters to strip.
    /// Ordered longest-first to ensure correct greedy matching.
    ///
    /// The strip length may differ from the match length: for `zxdg_` we strip
    /// only the `z` instability marker, preserving `xdg_` as part of the type
    /// name (matching existing C++ types like `XdgOutputManagerV1`).
    ///
    /// Prefixes NOT listed here (e.g., `xdg_`, `ext_`) are intentionally kept
    /// as part of the type name (e.g., `XdgActivationV1`, `ExtForeignToplevelListV1`).
    const KNOWN_PREFIXES: &[(&str, usize)] = &[
        ("org_kde_kwin_", "org_kde_kwin_".len()),
        ("zwlr_", "zwlr_".len()),
        ("zwp_", "zwp_".len()),
        ("zxdg_", 1), // strip only the 'z' instability marker
        ("wl_", "wl_".len()),
        ("wp_", "wp_".len()),
    ];

    for &(prefix, strip_len) in KNOWN_PREFIXES {
        if s.starts_with(prefix) {
            return &s[strip_len..];
        }
    }
    s
}

/// Formats the Wayland interface name to the name of the class that
/// provides its C++ implementation.
pub fn format_wayland_interface_to_cpp_class(s: &str) -> String {
    let s = strip_wayland_interface_prefix(s);
    format!("{}", snake_to_pascal(s))
}

/// Formats the Wayland interface name to the name of the struct that
/// provides its Rust extension implementation.
pub fn format_wayland_interface_to_rust_extension_struct(s: &str) -> String {
    let s = strip_wayland_interface_prefix(s);
    format!("{}Middleware", snake_to_pascal(s))
}

pub fn format_has_arg(s: &str) -> String {
    format!("has_{}", dash_to_snake(s))
}

pub fn format_has_arg_ident(s: &str) -> Ident {
    format_ident!("has_{}", dash_to_snake(s))
}
/// Generate the namespace token for a protocol (either wayland_server::protocol or protocols::module_name).
pub fn generate_namespace(protocol: &WaylandProtocol) -> TokenStream {
    if protocol.name == "wayland" {
        quote! { wayland_server::protocol }
    } else {
        let protocol_module = dash_to_snake_ident(&protocol.name);
        quote! { protocols::#protocol_module }
    }
}
