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

use serde::{de, Deserialize, Deserializer, Serialize};
use std::fs;

fn main() {
    cxx_build::bridges(vec!["src/lib.rs"]).compile("wayland_rs");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");

    // Parse Wayland protocol XML files.
    let _protocols: Vec<WaylandProtocol> = {
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
                protocols.push(protocol);
            }
        }
        protocols
    };
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
