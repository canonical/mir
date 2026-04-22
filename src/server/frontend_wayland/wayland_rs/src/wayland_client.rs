use crate::wayland_server_core::ClientState;
use wayland_server::{backend::ClientId, Client, DisplayHandle};

/// A C++ friendly wrapper around a wayland [Client] object.
#[derive(Clone)]
pub struct WaylandClient {
    client: Client,
    handle: DisplayHandle,
}

impl WaylandClient {
    pub fn new(client: Client, handle: DisplayHandle) -> Self {
        WaylandClient { client, handle }
    }

    /// Retrieve the pid of the client.
    pub fn pid(&self) -> Result<i32, Box<dyn std::error::Error>> {
        Ok(self
            .client
            .get_credentials(&self.handle)
            .map_err(|e| format!("Failed to get client credentials: {e}"))?
            .pid)
    }

    /// Retrieve the uid of the client.
    pub fn uid(&self) -> Result<u32, Box<dyn std::error::Error>> {
        Ok(self
            .client
            .get_credentials(&self.handle)
            .map_err(|e| format!("Failed to get client credentials: {e}"))?
            .uid)
    }

    /// Retrieve the gid of the client.
    pub fn gid(&self) -> Result<u32, Box<dyn std::error::Error>> {
        Ok(self
            .client
            .get_credentials(&self.handle)
            .map_err(|e| format!("Failed to get client credentials: {e}"))?
            .gid)
    }

    /// Retrieve the [WaylandClientId] for this client.
    pub fn id(&self) -> Box<WaylandClientId> {
        Box::new(WaylandClientId::new(self.client.id()))
    }

    /// Retrieve the socket file descriptor for this client's connection.
    pub fn socket_fd(&self) -> Result<i32, Box<dyn std::error::Error>> {
        self.client
            .get_data::<ClientState>()
            .map(|state| state.fd())
            .ok_or_else(|| "Failed to get client state".into())
    }

    /// Retrieve the name of the client process, if available.
    ///
    /// This reads `/proc/{pid}/comm` to get the process name.
    pub fn name(&self) -> Result<String, Box<dyn std::error::Error>> {
        let pid = self.pid()?;
        let name = std::fs::read_to_string(format!("/proc/{pid}/comm"))
            .map(|s| s.trim_end().to_string())
            .map_err(|e| format!("Failed to read client name: {e}"))?;
        Ok(name)
    }

    /// Clone this client into a new [Box].
    pub fn clone_box(&self) -> Box<WaylandClient> {
        Box::new(self.clone())
    }
}

/// An opaque ID for the WaylandClient.
///
/// This is useful when a client is removed because we cannot resolve
/// a WaylandClient from a raw [ClientId]. Instead, the C++ side of things
/// can use [WaylandClient::equals] to check if this id matches one of their
/// clients.
pub struct WaylandClientId {
    id: ClientId,
}

impl WaylandClientId {
    pub fn new(id: ClientId) -> WaylandClientId {
        WaylandClientId { id }
    }

    /// Check if this id is the same as the other id.
    pub fn equals(&self, id: &Box<WaylandClientId>) -> bool {
        self.id == id.id
    }
}
