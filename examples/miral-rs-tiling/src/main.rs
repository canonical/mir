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

//! Minimal tiling window manager using the miral Rust API.
//!
//! This example demonstrates:
//! - Implementing the `WindowManagementPolicy` trait
//! - Using the `Advice` enum for lifecycle notifications
//! - External storage pattern for per-application data
//! - Builder-pattern `MirRunner` configuration
//!
//! Windows are tiled in a simple horizontal split layout. Each new window
//! gets an equal fraction of the available screen width.

use miral::prelude::*;
use std::collections::HashMap;

/// Per-application tile data stored externally in the policy struct.
#[derive(Debug, Clone)]
struct TileInfo {
    /// The assigned rectangle for this application's windows.
    rect: Rectangle,
}

/// A simple tiling window management policy.
///
/// All windows are tiled horizontally in equal-width columns.
struct TilingPolicy {
    /// The window manager tools for performing actions.
    tools: WindowManagerTools,
    /// The current application zone (usable area).
    zone: Rectangle,
    /// Per-application tile assignments (external storage pattern).
    tiles: HashMap<u64, TileInfo>,
    /// Ordered list of application IDs for consistent tiling order.
    app_order: Vec<u64>,
    /// Launcher for spawning external client applications.
    launcher: ExternalClientLauncher,
}

impl TilingPolicy {
    /// Recompute tile positions for all tracked applications.
    fn retile(&mut self) {
        let count = self.app_order.len();
        if count == 0 {
            return;
        }

        let tile_width = self.zone.size.width / count as i32;
        let tile_height = self.zone.size.height;

        for (i, app_id) in self.app_order.iter().enumerate() {
            let rect = Rectangle::new(
                Point::new(
                    self.zone.top_left.x + tile_width * i as i32,
                    self.zone.top_left.y,
                ),
                Size::new(tile_width, tile_height),
            );
            self.tiles.insert(*app_id, TileInfo { rect });
        }
    }
}

impl Default for TilingPolicy {
    fn default() -> Self {
        Self {
            tools: WindowManagerTools::uninit(),
            zone: Rectangle::default(),
            tiles: HashMap::new(),
            app_order: Vec::new(),
            launcher: ExternalClientLauncher::default(),
        }
    }
}

impl WindowManagementPolicy for TilingPolicy {
    fn tools(&self) -> &WindowManagerTools {
        &self.tools
    }

    fn tools_mut(&mut self) -> &mut WindowManagerTools {
        &mut self.tools
    }

    fn place_new_window(
        &mut self,
        app_info: &ApplicationInfo,
        requested: &WindowSpecification,
    ) -> WindowSpecification {
        // Look up the tile for this application
        let app_id = app_info.application().id();
        if let Some(tile) = self.tiles.get(&app_id) {
            requested
                .clone()
                .with_top_left(tile.rect.top_left)
                .with_size(tile.rect.size)
                .with_state(WindowState::Restored)
        } else {
            // Unknown app — use the full zone
            requested
                .clone()
                .with_top_left(self.zone.top_left)
                .with_size(self.zone.size)
        }
    }

    fn handle_modify_window(
        &mut self,
        window_info: &WindowInfo,
        modifications: &WindowSpecification,
    ) {
        self.tools()
            .modify_window(window_info.window(), modifications);
    }

    fn handle_window_ready(&mut self, window_info: &WindowInfo) {
        self.tools().select_active_window(window_info.window());
    }

    fn handle_raise_window(&mut self, window_info: &WindowInfo) {
        // Allow raising but don't change tile layout
        self.tools().select_active_window(window_info.window());
    }

    fn handle_keyboard_event(&mut self, event: &KeyboardEvent) -> bool {
        // Super+Enter: launch miral-terminal
        if event.action == KeyAction::Down && event.modifiers.alt() && event.keysym == 0xff0d
        // XKB_KEY_Return
        {
            let _ = self.launcher.launch("konsole");
            return true;
        }

        // Super+Q: close focused window
        if event.action == KeyAction::Down && event.modifiers.alt() && event.keysym == 0x71
        // XKB_KEY_q
        {
            if let Some(window) = self.tools().active_window() {
                self.tools().ask_client_to_close(&window);
            }
            return true;
        }

        false
    }

    fn handle_pointer_event(&mut self, event: &PointerEvent) -> bool {
        if event.action == PointerAction::ButtonDown {
            let point = Point::new(event.x as i32, event.y as i32);
            if let Some(window) = self.tools().window_at(point) {
                self.tools().select_active_window(&window);
            }
        }
        false
    }

    fn advise(&mut self, event: Advice) {
        match event {
            Advice::NewApp { ref app } => {
                let app_id = app.application().id();
                self.app_order.push(app_id);
                self.retile();
            }
            Advice::DeleteApp { ref app } => {
                let app_id = app.application().id();
                self.app_order.retain(|id| *id != app_id);
                self.tiles.remove(&app_id);
                self.retile();
            }
            Advice::ZoneCreate { ref zone } => {
                self.zone = zone.extents();
                self.retile();
            }
            Advice::ZoneUpdate { ref updated, .. } => {
                self.zone = updated.extents();
                self.retile();
            }
            _ => {}
        }
    }
}

fn main() {
    let launcher = ExternalClientLauncher::new();
    let launcher_for_policy = launcher.clone();

    let result = MirRunner::new(std::env::args())
        .add(Decorations::prefer_csd())
        .add(WaylandExtensions::default())
        .add(launcher)
        .add(Magnifier::default())
        .add_window_management_policy_with(move || TilingPolicy {
            launcher: launcher_for_policy,
            ..Default::default()
        })
        .on_start(|| println!("Tiling WM started"))
        .on_stop(|| println!("Tiling WM stopped"))
        .run();

    if let Err(e) = result {
        eprintln!("Error: {}", e);
        std::process::exit(1);
    }
}
