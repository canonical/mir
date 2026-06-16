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
use calloop::channel::{channel, Event as ChannelEvent, Sender};
use calloop::ping::Ping;
use calloop::{generic::Generic, EventLoop, Interest, Mode, PostAction};
use cxx::UniquePtr;
use log;
use std::error;
use std::option::Option;
use std::sync::{mpsc, Arc, Mutex};
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, DisplayHandle, ListeningSocket,
};

/// A function queued to run on the server's event loop thread.
///
/// The function is given mutable access to the [`ServerState`] so that it can
/// interact with the running server (e.g. to call back into C++).
type ScheduledFn = Box<dyn FnOnce(&mut ServerState) + Send>;

/// The wayland server.
pub struct WaylandServer {
    stop_signal: Mutex<Option<Ping>>,
    task_sender: Mutex<Option<Sender<ScheduledFn>>>,
}

impl WaylandServer {
    /// Create a new wayland server.
    pub fn new() -> Self {
        WaylandServer {
            stop_signal: Mutex::new(None),
            task_sender: Mutex::new(None),
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
        let (pinger, ping_source) = calloop::ping::make_ping()?;

        *self
            .stop_signal
            .lock()
            .expect("No recovery from lock poisioning") = Some(pinger);

        loop_handle
            .insert_source(ping_source, |_, _, server: &mut ServerState| {
                server.stop_requested = true;
            })
            .map_err(|_| "Failed to insert stop eventfd into event loop")?;

        // Add a queue so that arbitrary functions can be scheduled (from any
        // thread) to run on the event loop thread. Functions are executed in
        // the order they were queued.
        let (task_sender, task_channel) = channel::<ScheduledFn>();

        *self
            .task_sender
            .lock()
            .expect("No recovery from lock poisioning") = Some(task_sender);

        loop_handle
            .insert_source(task_channel, |event, _, state: &mut ServerState| {
                if let ChannelEvent::Msg(task) = event {
                    task(state);
                }
            })
            .map_err(|_| "Failed to insert task queue into event loop")?;

        WaylandServer::register_globals(&state, Arc::new(Mutex::new(factory)));

        // TODO: Don't spin continuously
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
            .expect("No way to recover from lock poisioning")
            .as_ref()
        {
            signal.ping();
        }
    }

    /// Schedule a function to be executed on the event loop thread.
    ///
    /// The function is queued and will run on the thread driving `run`, in the
    /// order it was scheduled relative to other scheduled functions. This may
    /// be called from any thread. Any number of functions may be queued.
    ///
    /// Returns `true` if the function was queued, or `false` if the server is
    /// not currently running (i.e. `run` has not been called, or the event
    /// loop has already torn down its task queue).
    fn schedule(&self, task: ScheduledFn) -> bool {
        match self
            .task_sender
            .lock()
            .expect("No way to recover from lock poisioning")
            .as_ref()
        {
            Some(sender) => sender.send(task).is_ok(),
            None => false,
        }
    }

    /// Schedule a unit of C++ work to be run on the event loop thread.
    ///
    /// The work itself is owned by the C++ side and identified by `work_id`.
    /// When the event loop processes the queued task it calls back into C++
    /// via `WorkCallback::execute(work_id)`, on the event-loop thread.
    ///
    /// This may be called from any thread, and any number of units of work may
    /// be queued; they are run in the order they were scheduled.
    pub fn schedule_work(&mut self, work_id: u32) {
        self.schedule(Box::new(move |state: &mut ServerState| {
            state.work_callback.pin_mut().execute(work_id);
        }));
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
struct ClientState {
    on_disconnect: Box<dyn Fn(ClientId, DisconnectReason) + Send + Sync>,
}

impl ClientData for ClientState {
    fn initialized(&self, _client_id: ClientId) {}

    fn disconnected(&self, client_id: ClientId, reason: DisconnectReason) {
        (self.on_disconnect)(client_id, reason);
    }
}
