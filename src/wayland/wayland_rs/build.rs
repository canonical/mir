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

use proc_macro2::TokenStream;
use quote::{format_ident, quote};
use serde::{de, Deserialize, Deserializer, Serialize};
use std::fs;

fn main() {
    cxx_build::bridges(vec!["src/lib.rs"]).compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");

    // Parse Wayland protocol XML files.
    let protocols: Vec<(WaylandProtocolMetadata, WaylandProtocol)> = {
        let mut protocols = Vec::new();
        let paths = fs::read_dir("../../../wayland-protocols").unwrap();
        for file in paths {
            if let Ok(entry) = file {
                let xml_content =
                    fs::read_to_string(entry.path()).expect("Failed to read protocol XML file");
                let protocol: WaylandProtocol = quick_xml::de::from_str(&xml_content)
                    .map_err(|e| {
                        panic!(
                            "Failed to parse protocol XML file {}: {}",
                            entry.path().display(),
                            e
                        )
                    })
                    .unwrap();
                let mut metadata: WaylandProtocolMetadata = WaylandProtocolMetadata {
                    path: fs::canonicalize(entry.path())
                        .unwrap()
                        .to_str()
                        .unwrap()
                        .to_string(),
                    dependencies: Vec::new(),
                };

                fn add_interface_for_arg(arg: &WaylandArg, metadata: &mut WaylandProtocolMetadata) {
                    if let Some(interface_name) = &arg.interface {
                        metadata.dependencies.push(interface_name.clone());
                    }
                    if let Some(enm) = &arg.enum_ {
                        let split = enm.split(".").collect::<Vec<&str>>();
                        if split.len() == 2 {
                            let interface_name = split[0];
                            metadata.dependencies.push(interface_name.to_string());
                        }
                    }
                }

                // Collect dependencies from interfaces
                for interface in &protocol.interfaces {
                    for item in &interface.items {
                        match item {
                            InterfaceItem::Request(req) => {
                                for arg in &req.args {
                                    add_interface_for_arg(arg, &mut metadata);
                                }
                            }
                            InterfaceItem::Event(evt) => {
                                for arg in &evt.args {
                                    add_interface_for_arg(arg, &mut metadata);
                                }
                            }
                            _ => {}
                        }
                    }
                }

                protocols.push((metadata, protocol));
            }
        }
        protocols
    };

    // Generate the Wayland scanner protocol generator code for all interfaces
    // except the core Wayland protocol.
    let generated_protocols = protocols.iter().map(|(metadata, protocol)| {
        // We rely on the wayland_server crate for the core Wayland protocol.
        if protocol.name == "wayland" {
            return quote! {};
        }

        let struct_name = format_ident!("{}", protocol.name.replace('-', "_"));
        let path = &metadata.path;

        let mut server_code_use_statements = quote! {
            use super::wayland_server;
            use self::interfaces::*;
        };

        // Add use statements for other protocol dependencies
        let protocol_dependencies: std::collections::HashSet<String> = metadata
            .dependencies
            .iter()
            .filter_map(|dependency| {
                protocols
                    .iter()
                    .find(|(_, dep_protocol)| {
                        dep_protocol.name != protocol.name
                            && dep_protocol
                                .interfaces
                                .iter()
                                .any(|iface| iface.name == *dependency)
                    })
                    .map(|(_, dep_protocol)| dep_protocol.name.clone())
            })
            .collect();

        for dependency in &protocol_dependencies {
            let dep_struct_name = format_ident!("{}", dependency.replace('-', "_"));
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
            let dep_struct_name = format_ident!("{}", dependency.replace('-', "_"));
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
                        use wayland_server::protocol::__interfaces::*;
                    };
                } else {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
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
        use wayland_scanner;
        use wayland_server;

        #(#generated_protocols)*
    };

    write_generated_rust_file(generated_protocol_rs, "protocols.rs");
}

/// Write the generated Rust code to a file with proper formatting.
fn write_generated_rust_file(tokens: proc_macro2::TokenStream, filename: &str) {
    let out_dir = "src";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    let syntax_tree: syn::File = syn::parse2(tokens).unwrap();
    let formatted_code = prettyplease::unparse(&syntax_tree);

    fs::write(dest_path, formatted_code).unwrap();
}

/// Representation of a Wayland protocol XML file.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandProtocol {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "interface")]
    interfaces: Vec<WaylandInterface>,
}

/// Representation of a Wayland interface in a protocol XML file.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandInterface {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "@version")]
    version: u32,

    // $value tells quick-xml to put ALL child elements into this Vec,
    // maintaining their exact order.
    //
    // We cannot just use separate Vecs for requests, events, enums, etc.
    // because serde expects that all elements of the same type are grouped together,
    // which is not the case in Wayland protocol XML files.
    #[serde(rename = "$value")]
    items: Vec<InterfaceItem>,
}

/// An enum representing the possible child elements of an interface.
/// The `rename_all = "lowercase"` maps the XML tag names (e.g., <request>)
/// to the enum variants (e.g., Request).
#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
enum InterfaceItem {
    Description(WaylandDescription),
    Request(WaylandRequest),
    Event(WaylandEvent),
    Enum(WaylandEnum),
}

/// Representation of a description tag
#[derive(Debug, Serialize, Deserialize)]
struct WaylandDescription {
    #[serde(rename = "@summary")]
    summary: String,

    #[serde(rename = "$value", default)]
    text: String,
}

/// Representation of a Wayland request on an interface.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandRequest {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "@type")]
    type_: Option<String>,

    #[serde(rename = "@since")]
    since: Option<u32>,

    // Descriptions can appear inside requests too
    #[serde(rename = "description", default)]
    description: Option<WaylandDescription>,

    #[serde(rename = "arg", default)]
    args: Vec<WaylandArg>,
}

/// Representation of a Wayland event on an interface.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandEvent {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "@type")]
    type_: Option<String>,

    #[serde(rename = "@since")]
    since: Option<u32>,

    #[serde(rename = "description", default)]
    description: Option<WaylandDescription>,

    #[serde(rename = "arg", default)]
    args: Vec<WaylandArg>,
}

/// Representation of an argument on a Wayland request or event.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandArg {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "@type")]
    type_: String,

    #[serde(rename = "@enum")]
    enum_: Option<String>,

    #[serde(rename = "@interface")]
    interface: Option<String>,

    #[serde(rename = "@allow-null", default)]
    allow_null: Option<bool>,
}

/// Representation of an enum.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandEnum {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "@bitfield")]
    bitfield: Option<bool>,

    #[serde(rename = "description", default)]
    description: Option<WaylandDescription>,

    #[serde(rename = "entry")]
    entries: Vec<WaylandEnumEntry>,
}

// Helper function to handle both "123" and "0x7b"
fn deserialize_int_or_hex<'de, D>(deserializer: D) -> Result<i32, D::Error>
where
    D: Deserializer<'de>,
{
    let s: String = Deserialize::deserialize(deserializer)?;
    if s.starts_with("0x") || s.starts_with("0X") {
        // Parse as base 16 (hex)
        i32::from_str_radix(&s[2..], 16).map_err(de::Error::custom)
    } else {
        // Parse as base 10
        s.parse::<i32>().map_err(de::Error::custom)
    }
}

/// Representation of an entry in a Wayland enum.
#[derive(Debug, Serialize, Deserialize)]
struct WaylandEnumEntry {
    #[serde(rename = "@name")]
    name: String,

    #[serde(rename = "@value", deserialize_with = "deserialize_int_or_hex")]
    value: i32,

    #[serde(rename = "@since", default)]
    since: Option<u32>,

    #[serde(rename = "@summary", default)]
    summary: Option<String>,
}

struct WaylandProtocolMetadata {
    path: String,
    dependencies: Vec<String>, // interface names this protocol depends on
}
