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

//! `mir_wayland_generator` generates typed C++ wrappers for a Wayland protocol that Mir
//! does not implement itself.
//!
//! The wrappers are compiled into the code implementing the protocol. At runtime they
//! register a structured description of the protocol with Mir's Rust Wayland server
//! (`register_dynamic_protocol`) and forward its requests and events through the
//! generic `DynamicResource`/`DynamicResourceHandler` bridge. The protocol XML is only
//! read here, at build time.

#[allow(dead_code)]
#[path = "../../build_script/protocol_parser.rs"]
mod protocol_parser;

mod emit;
mod model;

use std::{fs, path::Path, process::ExitCode};

const USAGE: &str = "Usage: mir_wayland_generator <prefix> <protocol.xml> header|source [output]

  <prefix>  is stripped from interface names before they become class names
            (\"_\" strips nothing).
  [output]  is the file to write; standard output if omitted.

The source includes the header as \"<protocol stem>_wrapper.h\".";

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().collect();
    if !(4..=5).contains(&args.len()) {
        eprintln!("{USAGE}");
        return ExitCode::from(2);
    }

    match run(
        &args[1],
        &args[2],
        &args[3],
        args.get(4).map(String::as_str),
    ) {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("mir_wayland_generator: {}: {error}", args[2]);
            ExitCode::FAILURE
        }
    }
}

fn run(prefix: &str, xml_path: &str, kind: &str, output: Option<&str>) -> Result<(), String> {
    let xml = fs::read_to_string(xml_path).map_err(|e| e.to_string())?;
    let parsed: protocol_parser::WaylandProtocol =
        quick_xml::de::from_str(&xml).map_err(|e| format!("invalid protocol XML: {e}"))?;
    let protocol = model::Protocol::new(&parsed, prefix)?;

    let path = Path::new(xml_path);
    let file_name = path
        .file_name()
        .map(|name| name.to_string_lossy().into_owned())
        .unwrap_or_default();
    let stem = path
        .file_stem()
        .map(|stem| stem.to_string_lossy().into_owned())
        .unwrap_or_default();

    let text = match kind {
        "header" => emit::header(&protocol, &file_name),
        "source" => emit::source(&protocol, &file_name, &format!("{stem}_wrapper.h")),
        _ => return Err(format!("unknown output kind \"{kind}\"\n\n{USAGE}")),
    };

    match output {
        Some(output) => fs::write(output, text).map_err(|e| format!("{output}: {e}")),
        None => {
            print!("{text}");
            Ok(())
        }
    }
}
