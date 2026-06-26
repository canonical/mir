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

use crate::ffi::{GlobalFactory, WaylandServerNotificationHandler, WorkCallback};
use crate::wayland_client::{WaylandClient, WaylandClientId};
use calloop::ping::Ping;
use calloop::{generic::Generic, EventLoop, Interest, Mode, PostAction};
use cxx::UniquePtr;
use log;
use std::error;
use std::option::Option;
use std::os::fd::AsRawFd;
use std::sync::{mpsc, Arc, Mutex};
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, DisplayHandle, ListeningSocket,
};

/// The wayland server.
pub struct WaylandServer {
    stop_signal: Mutex<Option<Ping>>,
    work_signal: Mutex<Option<Ping>>,
}

impl WaylandServer {
    /// Create a new wayland server.
    pub fn new() -> Self {
        WaylandServer {
            stop_signal: Mutex::new(None),
            work_signal: Mutex::new(None),
        }
    }

    /// Run the wayland server.
    ///
    /// This function will block the current thread.
    ///
    /// # Arguments
    /// * `socket` - The name of the socket to bind to (e.g. "wayland-0").
    pub fn run(
        &mut self,
        socket: &str,
        factory: UniquePtr<GlobalFactory>,
        notification_handler: UniquePtr<WaylandServerNotificationHandler>,
        work_callback: UniquePtr<WorkCallback>,
    ) -> Result<(), Box<dyn error::Error>> {
        let display = Display::<ServerState>::new()?;
        let mut event_loop: EventLoop<'_, ServerState> = EventLoop::try_new()?;
        let loop_handle = event_loop.handle();
        let (disconnect_tx, disconnect_rx) = mpsc::channel::<(ClientId, DisconnectReason)>();
        let mut state = ServerState {
            handle: display.handle(),
            stop_requested: false,
            notification_handler,
            work_callback,
            disconnect_rx,
        };

        // First, add the listener to the event loop.
        let listener = ListeningSocket::bind(socket)?;
        loop_handle.insert_source(
            Generic::new(listener, Interest::READ, Mode::Level),
            move |_, listener, state: &mut ServerState| {
                if let Ok(stream) = listener.accept() {
                    if let Some(stream) = stream {
                        // Insert the client into the display.
                        // This registers the client's socket with the Display's internal backend.
                        let disconnect_tx = disconnect_tx.clone();
                        let client_state = ClientState {
                            on_disconnect: Box::new(move |client_id, reason| {
                                log::info!(
                                    "Client disconnected: {:?}, reason: {:?}",
                                    client_id,
                                    reason
                                );
                                // We publish the notification about the disconnected client onto the channel
                                // so that the WaylandServerNotificationHandler is guaranteed to only be
                                // spoken to on a single thread.
                                let _ = disconnect_tx.send((client_id, reason));
                            }),
                            socket_fd: stream.as_raw_fd(),
                        };
                        match state.handle.insert_client(stream, Arc::new(client_state)) {
                            Err(e) => log::error!("Failed to add client: {}", e),
                            Ok(client) => {
                                // Notify C++ that we have a new WaylandClient available to us.
                                // The C++ side of things can choose to hold onto this Box if they
                                // choose to.
                                let wayland_client =
                                    WaylandClient::new(client.clone(), state.handle.clone());
                                state
                                    .notification_handler
                                    .pin_mut()
                                    .client_added(Box::new(wayland_client));
                            }
                        }
                    }
                }
                Ok(PostAction::Continue)
            },
        )?;

        loop_handle.insert_source(
            Generic::new(display, Interest::READ, Mode::Level),
            |_, server, state: &mut ServerState| {
                // This function processes requests from all connected clients

                // SAFETY:
                // NoIoDrop<Display> exposes `get_mut` as unsafe because we could use a mutable
                // reference to drop or close the underlying IO object.
                // We are not doing that; this upholds the required invariant
                unsafe {
                    if let Err(e) = server.get_mut().dispatch_clients(state) {
                        log::error!("Error dispatching wayland clients: {}", e);
                    }
                }
                Ok(PostAction::Continue)
            },
        )?;

        // Add stop_signal to wake and terminate the loop.
        let (stop_pinger, stop_ping_source) = calloop::ping::make_ping()?;

        *self
            .stop_signal
            .lock()
            .expect("No recovery from lock poisoning") = Some(stop_pinger);

        loop_handle
            .insert_source(stop_ping_source, |_, _, server: &mut ServerState| {
                server.stop_requested = true;
            })
            .map_err(|_| "Failed to insert stop eventfd into event loop")?;

        // Add a work signal so that C++ can ask (from any thread) for its
        // pending work to be drained on the event loop thread. The ping
        // coalesces multiple requests into a single wake; when it fires we ask
        // the C++ side to run all the work it currently has queued.
        let (work_pinger, work_ping_source) = calloop::ping::make_ping()?;

        *self
            .work_signal
            .lock()
            .expect("No recovery from lock poisoning") = Some(work_pinger);

        loop_handle
            .insert_source(work_ping_source, |_, _, state: &mut ServerState| {
                state.work_callback.pin_mut().execute();
            })
            .map_err(|_| "Failed to insert work signal into event loop")?;

        WaylandServer::register_globals(&state, Arc::new(Mutex::new(factory)));

        // Drain any work the C++ side queued before the work signal was ready.
        // Such a signal would have been dropped (the queue is owned by C++ and
        // outlives `run`), so run that work now that the loop is starting. Work
        // queued from here on raises its own signal and is handled by the loop.
        state.work_callback.pin_mut().execute();

        while !state.stop_requested {
            // 1. Dispatch events
            // The event loop borrows `server` temporarily to run the callbacks
            event_loop
                .dispatch(None, &mut state)
                .map_err(|_| "Failed to dispatch event loop")?;

            // 2. Process any pending disconnects
            while let Ok((client_id, _reason)) = state.disconnect_rx.try_recv() {
                state
                    .notification_handler
                    .pin_mut()
                    .client_removed(Box::new(WaylandClientId::new(client_id)));
            }

            // 3. Flush clients
            // Because `event_loop.dispatch` only borrowed `server`, we get it back here.
            state
                .handle
                .flush_clients()
                .map_err(|_| "Failed to flush clients")?;
        }

        Ok(())
    }

    /// Stops the wayland server if it is running.
    pub fn stop(&self) {
        if let Some(signal) = self
            .stop_signal
            .lock()
            .expect("No way to recover from lock poisoning")
            .as_ref()
        {
            signal.ping();
        }
    }

    /// Signal the event loop to drain any work the C++ side has queued.
    ///
    /// The work itself is owned by the C++ side. When the event loop processes
    /// this signal it calls back into C++ via `WorkCallback::execute()`, on the
    /// event-loop thread, which runs all of the C++ work currently pending.
    ///
    /// This may be called from any thread. Signals coalesce, so the C++ side
    /// need only call this while it has no signal already outstanding.
    ///
    /// Returns `true` if the signal was delivered, or `false` if the server is
    /// not currently running (i.e. `run` has not been called yet) and the
    /// signal was dropped. The caller is responsible for re-signalling later in
    /// the latter case.
    pub fn drain_queue(&self) -> bool {
        match self
            .work_signal
            .lock()
            .expect("No way to recover from lock poisoning")
            .as_ref()
        {
            Some(pinger) => {
                pinger.ping();
                true
            }
            None => false,
        }
    }
}

/// Create a new wayland server.
pub fn create_wayland_server() -> Box<WaylandServer> {
    Box::new(WaylandServer::new())
}

/// The state of the wayland server.
pub struct ServerState {
    pub handle: DisplayHandle,
    stop_requested: bool,
    notification_handler: UniquePtr<WaylandServerNotificationHandler>,
    work_callback: UniquePtr<WorkCallback>,
    disconnect_rx: mpsc::Receiver<(ClientId, DisconnectReason)>,
}

/// The state of a wayland client.
pub struct ClientState {
    on_disconnect: Box<dyn Fn(ClientId, DisconnectReason) + Send + Sync>,
    socket_fd: std::os::fd::RawFd,
}

impl ClientState {
    /// Retrieve the socket file descriptor for this client's connection.
    pub fn fd(&self) -> i32 {
        self.socket_fd
    }
}

impl ClientData for ClientState {
    fn initialized(&self, _client_id: ClientId) {}

    fn disconnected(&self, client_id: ClientId, reason: DisconnectReason) {
        (self.on_disconnect)(client_id, reason);
    }
}
