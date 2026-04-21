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

    /// Check if this client is wrapping the provided [WaylandClientId].
    pub fn equals(&self, id: &WaylandClientId) -> bool {
        self.client.id() == id.id
    }

    /// Retrieve the id of the client
    pub fn id(&self) -> Box<WaylandClientId> {
        Box::new(WaylandClientId::new(self.client.id()))
    }

    /// Clone the client to a new box.
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
}
