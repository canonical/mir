# Security

This document aims to explore in depth the security considerations around Mir
based compositors.

## Threat model

Mir is a C++ library for building compositors, not a product itself. As such,
when discussing the threat model for Mir, it is useful to discuss it in terms
of an actual product that is built on Mir. With this in mind, we will define
the threat model of **Ubuntu Frame** in this document.

### Ubuntu Frame threat model diagram

Ubuntu Frame is published as a snap. As such, the threat model for frame assumes
that the snap is secure, and proceeds to outline the frame snap's interactions
with the outside world.

**Diagram**: A flowchart depicting the security barries between compoinents in Ubuntu Frame.

```{mermaid} ubuntu_frame_threat_model.mmd
```

(security-event-logging)=

## Security event logging

The following events will be logged to standard error in [the OWASP format](https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Logging_Vocabulary_Cheat_Sheet.md):

- `sys_startup` on startup:
  ```json
  {"datetime": "YYYY-MM-DDThh:mm:ssZ", "appid": "mir.canonical.com", "event": "sys_startup", "level": "WARN", "description": "Mir is starting up" }
  ```
- `sys_shutdown` on shutdown:
  ```json
  {"datetime": "YYYY-MM-DDThh:mm:ssZ", "appid": "mir.canonical.com", "event": "sys_shutdown", "level": "WARN", "description": "Mir is shutting down" }
  ```
- `sys_crash` on unhandled error or fatal signal received:
  ```json
  {"datetime": "YYYY-MM-DDThh:mm:ssZ", "appid": "mir.canonical.com", "event": "sys_crash", "level": "ERROR", "description": "Mir unhandled exception in: <function>" }
  {"datetime": "YYYY-MM-DDThh:mm:ssZ", "appid": "mir.canonical.com", "event": "sys_crash", "level": "ERROR", "description": "Mir fatal signal received: <function>" }
  ```

## Cryptography

There is no cryptography used in Mir, no direct dependency on en/decryption,
hashing or digital signatures.
