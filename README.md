# FRER MLO — IEEE 802.1CB Frame Replication and Elimination for Reliability over Wi-Fi 7 Multi-Link Operation

An OMNeT++ simulation framework implementing **IEEE 802.1CB Frame Replication and Elimination for Reliability (FRER)** on top of **IEEE 802.11be (Wi-Fi 7)** with **Multi-Link Operation (MLO)**. 

---

## Dependencies

| Dependency | Version |
|---|---|
| [OMNeT++](https://omnetpp.org/) | **6.0.3** |
| [INET Framework](https://inet.omnetpp.org/) | **4.5.2** |
| [wifi-mlo-omnet](https://github.com/tkn-tub/wifi-mlo-omnet) | (see below) |

### MLO Base Framework

This project **requires** the MLO base framework as a linked OMNeT++ project:

> https://github.com/tkn-tub/wifi-mlo-omnet

The `Ieee8021CbStation` and `Ieee8021CbAccessPoint` nodes extend `MLOStation` and `MLOAccessPoint` defined in that framework. Both projects must reside in the same OMNeT++ workspace and the MLO project (`mlo`) must be set as a referenced project in the build settings.

## Building

This project is built using the **OMNeT++ IDE** (Eclipse-based):

1. Place this project folder, `inet4.5/`, and `wifi-mlo-omnet/` side by side in your OMNeT++ workspace.
2. Import all three projects into the workspace.
3. Set project Makemake properties for this project:
   - Root: **build** mode (makemake), source location
   - `simulations/` and `src/`: **No makefile**, C++ Source Folder
4. Add `wifi-mlo-omnet` as a referenced project under **Project > Properties > Project References**.
5. The MLO project should be set to **library** (static or shared) in its Makemake settings.
6. Build via **Project > Build** in the OMNeT++ IDE.

---

## Architecture

### Design Overview

FRER is layered between the application and the MLO U-MAC. The `Ieee8021rLayer` (from INET) handles stream identification, replication, and duplicate elimination. The FRER-aware U-MAC steers each replicated member stream to a dedicated radio link. The 802.1CB layer is kept unmodified; compatibility with the MLO stack is handled entirely in the surrounding modules.

```
Application (UdpBasicApp / UdpSink)
          ↕
   Ieee8021rLayer   (stream identification, splitting, merging — INET)
          ↕
  Ieee8021CbUMac   (per-stream link steering — src/umac/)
       ↕                  ↕
   lmac[0]            lmac[1]        (per-link IEEE 802.11be — MLO framework)
   2.4 GHz             5 GHz
```

**Uplink (STA → AP → STA):** The sender replicates each FRER packet into k member streams, tags each copy with an R-TAG (IEEE 802.1r Redundancy Tag) carrying a sequence number, and routes each copy to a distinct radio link via the forwarding table. At the AP, the first arriving copy is decoded, merged, relayed, and then re-split into k member streams for the downlink. The second arriving copy is discarded by the stream merger. At the receiver, the first copy is accepted and the second is eliminated.

### Modules

#### U-MAC (`src/umac/`)

- **`Ieee8021CbUMac`** — Extends the base `UMac` from the MLO framework. Implements FRER-aware packet routing:
  - *No R-TAG present:* packet has not yet passed through the 802.1CB pipeline — forwarded to the stream identifier for stream identification, sequence numbering, and splitting.
  - *R-TAG present, stream identified:* routes packet to the interface specified in `forwardingTable`.
  - *R-TAG present, no stream:* strips the R-TAG and forwards over the default interface (non-FRER frames).
  - At the AP on the downlink: converts `StreamInd`/`SequenceNumberInd` tags back to `StreamReq`/`SequenceNumberReq` so the 802.1CB layer can re-split the merged packet for downlink transmission over both links.

#### Relay (`src/relay/`)

- **`Ieee8021CbRelay`** — Extends INET's `Ieee8021dRelay`. Overrides lower-packet handling to preserve FRER-specific tags (`StreamInd`, `SequenceNumberInd`) when forwarding received frames, so that the U-MAC at the AP can identify and steer replicated streams correctly on the downlink.

#### L-MAC Portal (`src/lmac/portal/`)

- **`Ieee8021CbPortal`** — Extends INET's `Ieee80211Portal`. Overrides decapsulation to bridge the EPD/LPD header mismatch: the default WiFi LLC uses LPD-style framing, but FRER frames carry an EtherType-based R-TAG (EPD). The portal re-inserts the R-TAG into the reconstructed Ethernet frame and sets the required dispatch protocol tag so packets traverse the U-MAC correctly on the receive path.

#### Nodes (`src/node/`)

- **`Ieee8021CbStation`** — FRER-enabled Wi-Fi STA. Extends `MLOStation` with `Ieee8021CbUMac` and an `Ieee8021rLayer` for stream identification and duplicate elimination.
- **`Ieee8021CbAccessPoint`** — FRER-enabled Wi-Fi AP. Extends `MLOAccessPoint` with `Ieee8021CbUMac`, `Ieee8021CbRelay`, `Ieee8021CbPortal` (per-link portal), and an `Ieee8021rLayer`.

---

## Key Parameters

| Parameter | Location | Description |
|---|---|---|
| `umac.forwardingTable` | U-MAC | Maps member stream names to outgoing interface names. E.g. `{streamA1: "lmac0", streamA2: "lmac1"}` routes each replica to a different link. |
| `ieee8021r.policy.streamIdentifier.identifier.mapping` | `ieee8021r` | Identifies application flows as named streams and enables sequence numbering. Supports flexible packet filter expressions (UDP/TCP ports, IP fields). |
| `ieee8021r.policy.streamRelay.splitter.mapping` | `ieee8021r` | Splits a stream into member streams (one per link). E.g. `{streamA: ["streamA1", "streamA2"]}`. |
| `ieee8021r.policy.streamCoder.decoder.mapping` | `ieee8021r` | Identifies member streams on the receive side by MAC address and receiving interface (interface-based matching is critical for MLO). |
| `ieee8021r.policy.streamRelay.merger.mapping` | `ieee8021r` | Merges member streams and eliminates duplicates (VRA-like SRF). E.g. `{streamA1: "streamA", streamA2: "streamA"}`. Set to empty target at the listener to remove R-TAGs. |
| `ieee8021r.policy.streamRelay.merger.bufferSize` | `ieee8021r` | VRA sequence window size. Must be set large (≥ 500; 10000 used in experiments) due to sequence number divergence from asymmetric link speeds and wireless retransmissions. |
| `umac.defaultInterface` | U-MAC | Default outgoing interface for non-FRER (SLO) packets. |

> **Note on `StreamRedundancyConfigurator`:** INET's built-in `StreamRedundancyConfigurator` cannot be used here because it enforces VLAN/PCP-based stream configuration, which does not apply to plain WiFi. All FRER mappings must be configured manually per node in `omnetpp.ini`.

---

## Simulation Scenarios

Scenarios are defined in `simulations/omnetpp.ini`. All configurations share a common `[General]` base: two MLO STAs and one MLO AP operating over 2.4 GHz (52 Mbit/s) and 5 GHz (130 Mbit/s) links, scalar radio propagation with log-normal shadowing, 15 s simulation time, and 10 repetitions.

Three MLO operating modes are compared across scenarios:

| Mode | Description |
|---|---|
| **FRER** | Each packet is replicated and sent over both links; duplicate eliminated at receiver. |
| **Link Aggregation** | Frames distributed uniformly across links (`RandomUMac` from the base MLO project); no redundancy. |
| **SLO** | Single-link operation on the 2.4 GHz link only. |

### Configs

| Config | Network | Description |
|---|---|---|
| `SLO` | `BaseNetwork` | Baseline: UDP traffic over a single link (default 2.4 GHz). |
| `LinkAggregation` | `BaseNetwork` | MLO without FRER: frames distributed randomly across both links. |
| `FRER` | `BaseNetwork` | Full FRER: each packet replicated over both links, duplicate eliminated at receiver. Failure scenarios (Scenarios 1–4 in the paper) are run under this config by enabling the `ScenarioManager` script (`failure.xml`). |
| `Congestion` | `MixedNetwork` | FRER flow coexists with background SLO traffic from additional `MLOStation` nodes. Network size is swept from 8 to 24 STAs. |
| `Mobility` | `MixedNetwork` | FRER under mobility: all nodes follow `RandomWaypointMobility` at 7–9 m/s across a 450 × 450 m area. |

`MixedNetwork` extends `BaseNetwork` by adding arrays of plain `MLOStation` nodes (`slo_sender_sta`, `slo_receiver_sta`) as background traffic sources, parameterised by `networkSize`.

---

## Citation

If you use this framework in your research, please cite our paper:

```bibtex
@article{ergenc2026redundancy,
  author = {Ergen{\c{c}}, Do{\u{g}}analp and Reisinger, Tobias and Dressler, Falko},
  doi = {10.1016/j.comcom.2025.108373},
  title = {{Redundancy in WiFi 7: Combining Multi-link Operation with IEEE 802.1CB FRER}},
  journal = {Elsevier Computer Communications},
  issn = {0140-3664},
  publisher = {Elsevier},
  month = {2},
  volume = {247},
  year = {2026},
}
```

## License

This project is licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)**. See [LICENSE](LICENSE) for details.
