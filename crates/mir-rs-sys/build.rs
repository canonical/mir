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

fn main() {
    // System dependencies (miral and its transitive libraries) are declared
    // in the `[package.metadata.system-deps]` section of Cargo.toml. `probe()`
    // resolves them via pkg-config and emits the relevant cargo link directives.
    system_deps::Config::new().probe().expect(
        "\n\n\
        ERROR: Could not find the miral C++ library.\n\
        \n\
        The mir-sys crate requires the system-installed miral library (5.8 or later).\n\
        \n\
        On Ubuntu/Debian:\n\
        \n\
        sudo apt install libmiral-dev\n\
        \n\
        For other systems, see: https://github.com/canonical/mir/blob/main/HACKING.md\n\
        \n",
    );

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=src/bridge.h");
}
