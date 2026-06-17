# TSN Linux Host

Managing **Linux-based Time-Sensitive Networking (TSN) hosts** with the centralized,
programmable management approach of **Software-Defined Networking (SDN)**.

This project explores bringing SDN-style centralized configuration into TSN
environments so that Linux TSN end-stations (the *talkers* and *listeners* that
send and receive data streams) can be configured remotely and respond quickly to
configuration changes. It uses **NETCONF** for remote configuration, **YANG**
data models for structured config, **Sysrepo** as the datastore and
event-handling framework, and **Netopeer2** as the NETCONF server/client.

> Academic project — MSc Computer Science (International), University of Rostock.

---

## Background

- **SDN** separates the network *control plane* from the *data plane*, giving
  administrators centralized, programmable control of the whole network from a
  single SDN controller (e.g. via OpenFlow).
- **TSN** is a suite of IEEE 802.1 standards that add deterministic, low-latency,
  time-synchronized delivery to standard Ethernet — essential for automotive,
  industrial automation, and robotics.
- IEEE 802.1Qcc defines a **fully centralized** TSN architecture split between a
  Centralized User Configuration (**CUC**) and a Centralized Network
  Configuration (**CNC**). The CNC configures bridges (and, in this project, the
  Linux hosts) over a southbound interface using **NETCONF**.

The goal is to bridge SDN's flexibility with TSN's strict timing requirements,
simplifying configuration and administration of Linux TSN hosts.

See [`project.md`](project.md) for the full project description, task list, and a
curated set of reference links.

## Technologies

| Area | Tools |
|------|-------|
| Config protocol | NETCONF |
| Data modeling | YANG |
| Datastore & events | [Sysrepo](https://www.sysrepo.org/) |
| NETCONF server/client | [Netopeer2](https://github.com/CESNET/netopeer2) |
| Plugins | [telekom/sysrepo-plugins](https://github.com/telekom/sysrepo-plugins) |
| Environment | Docker (Ubuntu 24.04 LTS), Linux |
| Languages | C / C++ |

## Repository structure

```
.
├── Dockerfile                # Main build: libyang, Sysrepo, libnetconf2, Netopeer2 (+ plugins)
├── project.md                # Full project description, background, tasks & references
├── Event handler/            # Custom Sysrepo event handlers & example plugins
│   ├── Dockerfile
│   ├── hello_world_event_handler.c
│   ├── oven_event_handler.c
│   ├── example-config.xml
│   ├── oven-config.xml
│   ├── ReadMe                # Build/run notes for the event-handler container
│   └── results/              # Screenshots of plugin/handler output
└── resources/
    ├── instructions/         # Linux networking notes (tc qdisc, VLAN interfaces)
    ├── sysrepoplugins/        # Dockerfile for the telekom sysrepo-plugins setup
    └── udp/                   # TSN talker/listener test applications
        ├── send_udp.c         # UDP sender using tc-etf TxTime + SO_TIMESTAMPING
        ├── rec_udp.c          # UDP receiver with HW/SW timestamping
        └── README.txt         # Full flag reference and usage examples
```

## Getting started

The recommended environment is a Docker container, which builds Sysrepo,
Netopeer2, and their dependencies from source on Ubuntu 24.04.

```bash
# Build the image
docker build -t tsn-host .

# Run an interactive container
docker run -it --name tsn-host tsn-host
```

Inside the container:

```bash
# Set a root password (required for the Netopeer2 client connection)
passwd root

# Start the NETCONF server, then connect with the client in another shell
netopeer2-server
netopeer2-cli

# Verify the server is listening on the NETCONF port (830)
netstat -tulnp | grep 830
```

See [`Event handler/ReadMe`](Event%20handler/ReadMe) for the event-handler
container, where the example-module and oven plugins are pre-installed and ready
to run (event handlers in `/sysrepo/examples`, example XML configs in `/tmp`).

## Event handling

Configuration changes (defined via YANG models) must be detected and acted on
promptly to keep TSN behavior deterministic. Two approaches were explored:

1. **Custom Sysrepo event handlers** — subscribe directly to specific YANG
   modules via the Sysrepo API for full control (see the `hello_world` and
   `oven` handlers under [`Event handler/`](Event%20handler/)).
2. **Existing Sysrepo plugins** — reuse the well-tested
   [telekom/sysrepo-plugins](https://github.com/telekom/sysrepo-plugins)
   (e.g. `ietf-system-plugin`) for common system/interface/routing config.

## TSN talker/listener test apps

[`resources/udp/`](resources/udp/) contains UDP test applications used in the TSN
testbed. `send_udp` periodically transmits timestamped UDP packets using the
`tc-etf` TxTime feature and `SO_TIMESTAMPING`; `rec_udp` receives them with
hardware/software timestamps.

```bash
gcc -pthread -o /usr/local/bin/send_udp send_udp.c -lz
gcc -pthread -o /usr/local/bin/rec_udp  rec_udp.c  -lz

# Example
send_udp -i t1.42 -I t1 -d t2 -H
rec_udp  -i t2.42 -I t2 -H
```

A VLAN device with skb→PCP priority mapping is required. See
[`resources/udp/README.txt`](resources/udp/README.txt) for the complete flag
reference and [`resources/instructions/`](resources/instructions/) for VLAN and
`tc qdisc` setup notes.

## Status & known limitations

- Some Sysrepo plugins (notably `ietf-system-plugin`) depend on **systemd** and
  **DBus**, which are not fully available inside Docker. These plugins subscribe
  to changes successfully but fail when applying system-level changes because
  they cannot access the DBus service.
- Full integration of system-dependent plugins likely requires a **VM or bare
  metal** host where systemd is fully supported.

## Future work

1. Move from Docker to a VM or dedicated hardware for complete access to systemd
   and DBus.
2. Use custom plugins for entirely new configuration, and the telekom
   sysrepo-plugins for general system/interface/routing configuration.
3. Test on real TSN-enabled Linux hardware with VLAN interfaces / physical NICs
   to validate real-time performance and synchronization.

## Author

**Bibin Biju** — MSc Computer Science (International), University of Rostock.

Originally developed in the university GitLab as part of a team CSI project.
