# Mir SDK for Workshop

This SDK provides Mir's complete installed development and runtime tree under `/opt/mir`, including libraries, headers, platform plugins, and tools. Mir's development dependencies are installed in the workshop base, and consumer SDKs can access the Mir tree through a mount connection.

______________________________________________________________________

## Reference workshop

Add the `mir` SDK and wire a consumer SDK's mount plug to the Mir install slot:

```yaml
name: mir-development
base: ubuntu@26.04
sdks:
  - name: mir
    channel: latest/edge
  - name: my-compositor
connections:
  - plug: my-compositor:mir
    slot: mir:install
```

The consumer SDK must declare a `mount` plug named `mir` targeting `/opt/mir`.

______________________________________________________________________

## Using the SDK

### Prerequisites, project layout

1. Use a consumer SDK that declares the Mir mount plug shown above.
1. Include a C++ source file that uses the MirAL API.

### Build against MirAL

Once the workshop is ready, compile from inside it:

```bash
c++ main.cpp -o compositor $(pkg-config --cflags --libs miral)
```

Mir's installed tools, headers, libraries, and platform plugins are available under `/opt/mir` for the lifetime of the workshop. The SDK does not persist project files or caches.

______________________________________________________________________

## Plugs (resources this SDK consumes)

### `gpu`

- Interface: `gpu`
- Purpose: GPU access for Mir workloads; connects automatically when available.

## Slots (resources this SDK provides)

### `install`

- Interface: `mount`
- Workshop source: `$SDK/opt/mir`
- Purpose: Mir's complete installed development and runtime tree.

______________________________________________________________________

## Documentation and guidance

- [Mir documentation](https://mir-server.io/docs/)

______________________________________________________________________

## Community and support

- [Mir community](https://discourse.ubuntu.com/c/mir/)
- Please review our [Code of Conduct](https://ubuntu.com/community/ethos/code-of-conduct) before participating.

______________________________________________________________________

## Contributions

Contributions are welcome. See [HACKING.md](../../HACKING.md) and open issues or pull requests on the [Mir repository](https://github.com/canonical/mir).

______________________________________________________________________

## License and copyright

Copyright 2026 Canonical Ltd.

Mir contains components licensed under GPL-2.0, GPL-3.0, LGPL-2.1, and LGPL-3.0. See the [license files](../../) and source headers for the terms applicable to each component.
