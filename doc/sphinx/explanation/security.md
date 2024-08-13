# Threat Model
Mir is a C++ library for building compositors, not a product itself. As such,
when discussing the threat model for Mir, it is useful to discuss it in terms
of an actual product that is built on Mir. With this in mind, we will define
the threat model of **Ubuntu Frame** in this document.

## Ubuntu Frame Threat Model Diagram
Ubuntu Frame is published as a snap. As such, the threat model for frame assumes
that the snap is secure, and proceeds to outline the frame snap's interactions
with the outside world.

```{mermaid} ubuntu_frame_threat_model.mmd
```

# Cryptography
There is no cryptography used in Mir, no direct dependency on en/decryption,
hashing or digital signatures.
