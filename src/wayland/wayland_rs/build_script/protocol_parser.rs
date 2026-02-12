use serde::{de, Deserialize, Deserializer, Serialize};
use std::fs;

/// Representation of a Wayland protocol XML file.
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandProtocol {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "interface")]
    pub interfaces: Vec<WaylandInterface>,

    #[serde(skip)]
    pub path: String,

    #[serde(skip)]
    pub dependencies: Vec<String>, // interface names this protocol depends on
}

/// Representation of a Wayland interface in a protocol XML file.
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandInterface {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "@version")]
    pub version: u32,

    #[serde(skip)]
    pub is_global: bool,

    // $value tells quick-xml to put ALL child elements into this Vec,
    // maintaining their exact order.
    //
    // We cannot just use separate Vecs for requests, events, enums, etc.
    // because serde expects that all elements of the same type are grouped together,
    // which is not the case in Wayland protocol XML files.
    #[serde(rename = "$value")]
    pub items: Vec<InterfaceItem>,
}

/// An enum representing the possible child elements of an interface.
/// The `rename_all = "lowercase"` maps the XML tag names (e.g., <request>)
/// to the enum variants (e.g., Request).
#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum InterfaceItem {
    Description(WaylandDescription),
    Request(WaylandRequest),
    Event(WaylandEvent),
    Enum(WaylandEnum),
}

/// Representation of a description tag
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandDescription {
    #[serde(rename = "@summary")]
    pub summary: String,

    #[serde(rename = "$value", default)]
    pub text: String,
}

/// Representation of a Wayland request on an interface.
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandRequest {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "@type")]
    pub type_: Option<String>,

    #[serde(rename = "@since")]
    pub since: Option<u32>,

    // Descriptions can appear inside requests too
    #[serde(rename = "description", default)]
    pub description: Option<WaylandDescription>,

    #[serde(rename = "arg", default)]
    pub args: Vec<WaylandArg>,
}

/// Representation of a Wayland event on an interface.
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandEvent {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "@type")]
    pub type_: Option<String>,

    #[serde(rename = "@since")]
    pub since: Option<u32>,

    #[serde(rename = "description", default)]
    pub description: Option<WaylandDescription>,

    #[serde(rename = "arg", default)]
    pub args: Vec<WaylandArg>,
}

/// Representation of an argument on a Wayland request or event.
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandArg {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "@type")]
    pub type_: String,

    #[serde(rename = "@enum")]
    pub enum_: Option<String>,

    #[serde(rename = "@interface")]
    pub interface: Option<String>,

    #[serde(rename = "@allow-null", default)]
    pub allow_null: Option<bool>,
}

/// Representation of an enum.
#[derive(Debug, Serialize, Deserialize)]
pub struct WaylandEnum {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "@bitfield")]
    pub bitfield: Option<bool>,

    #[serde(rename = "description", default)]
    pub description: Option<WaylandDescription>,

    #[serde(rename = "entry")]
    pub entries: Vec<WaylandEnumEntry>,
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
pub struct WaylandEnumEntry {
    #[serde(rename = "@name")]
    pub name: String,

    #[serde(rename = "@value", deserialize_with = "deserialize_int_or_hex")]
    pub value: i32,

    #[serde(rename = "@since", default)]
    pub since: Option<u32>,

    #[serde(rename = "@summary", default)]
    pub summary: Option<String>,
}

pub fn parse_protocols() -> Vec<WaylandProtocol> {
    let mut protocols = Vec::new();
    let paths = fs::read_dir("../../../wayland-protocols").unwrap();
    for file in paths {
        if let Ok(entry) = file {
            let xml_content =
                fs::read_to_string(entry.path()).expect("Failed to read protocol XML file");
            let mut protocol: WaylandProtocol = quick_xml::de::from_str(&xml_content)
                .map_err(|e| {
                    panic!(
                        "Failed to parse protocol XML file {}: {}",
                        entry.path().display(),
                        e
                    )
                })
                .unwrap();
            protocol.path = fs::canonicalize(entry.path())
                .unwrap()
                .to_str()
                .unwrap()
                .to_string();
            protocol.dependencies = Vec::new();

            fn add_interface_for_arg(arg: &WaylandArg, dependencies: &mut Vec<String>) {
                if let Some(interface_name) = &arg.interface {
                    dependencies.push(interface_name.clone());
                }
                if let Some(enm) = &arg.enum_ {
                    let split = enm.split(".").collect::<Vec<&str>>();
                    if split.len() == 2 {
                        let interface_name = split[0];
                        dependencies.push(interface_name.to_string());
                    }
                }
            }

            // Collect dependencies from interfaces
            for interface in &protocol.interfaces {
                for item in &interface.items {
                    match item {
                        InterfaceItem::Request(req) => {
                            for arg in &req.args {
                                add_interface_for_arg(arg, &mut protocol.dependencies);
                            }
                        }
                        InterfaceItem::Event(evt) => {
                            for arg in &evt.args {
                                add_interface_for_arg(arg, &mut protocol.dependencies);
                            }
                        }
                        _ => {}
                    }
                }
            }

            // Check if the interface is global or not.
            // An interface is considered global if it is not created by any other interface's request or event.
            let global_interfaces: Vec<bool> = protocol
                .interfaces
                .iter()
                .map(|interface| {
                    let mut is_global = true;
                    for other_interface in &protocol.interfaces {
                        for item in &other_interface.items {
                            match item {
                                InterfaceItem::Request(req) => {
                                    for arg in &req.args {
                                        if let Some(interface_name) = &arg.interface {
                                            if interface_name == &interface.name
                                                && arg.type_ == "new_id"
                                            {
                                                is_global = false;
                                                break;
                                            }
                                        }
                                    }
                                }
                                _ => {}
                            }

                            if is_global == false {
                                break;
                            }
                        }
                    }
                    is_global
                })
                .collect();

            for (interface, is_global) in protocol.interfaces.iter_mut().zip(global_interfaces) {
                interface.is_global = is_global;
            }

            protocols.push(protocol);
        }
    }
    protocols
}
