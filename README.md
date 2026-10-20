<div align="center">

  <a href="https://attaky.com">
    <img alt="Attaky" src="./.github/attaky-logo-horizontal.png" width="32%" />
  </a>

  <h2>WadaMesh firmware for the Attaky Mesh series</h2>

  <p>
    <img src="https://img.shields.io/badge/board-Attaky_Core_1.0-C0252B?style=flat-square" alt="Attaky Core 1.0" />
    <a href="./LICENSE"><img src="https://img.shields.io/badge/license-GPL_3.0-blue?style=flat-square" alt="License" /></a>
    <a href="https://discord.attaky.com"><img src="https://img.shields.io/badge/Discord-attaky-5865F2?style=flat-square&logo=discord&logoColor=white" alt="Discord" /></a>
  </p>

  <a href="https://attaky.com">Attaky home</a>
  &nbsp;·&nbsp;
  <a href="https://discord.attaky.com">Discord</a>

</div>

> This repository is a fork of [WadaMesh](https://github.com/ALLFATHER-BV/wadamesh),
> originally developed by ALLFATHER-BV, last modified by Attaky on **2026-07-22** from
> upstream **`beta_47`**, to add support for the **Attaky Mesh series** of off-grid LoRa
> devices. Inherits WadaMesh's **GPL-3.0-or-later** license.

## Compatible devices

This release supports the Attaky Mesh series built on Attaky Core 1.0 with the LoRa + GPS
module:

- **Mesh Pocket Junior 1.0**
- **Mesh Deck Junior 1.0** *(with the integrated keyboard)*
- **Mesh Station Junior 1.0**

## What this fork adds on top of WadaMesh

- **Attaky Core 1.0 build target** — the `attaky_mesh_series_companion_radio_touch`
  PlatformIO environment
- **Display and touch** — the landscape touchscreen interface on the Attaky panel
- **LoRa and GPS** — off-grid mesh radio and positioning on the Attaky LoRa + GPS module
- **Battery telemetry** — battery level from the Attaky power modules
- **Hardware keyboard** — the optional Attaky keyboard module alongside the on-screen
  keyboard, with its symbol key summoning the on-screen keys

The Attaky board code lives in its own variant and build environment. Two small fixes in the
shared touch UI (a key typed right after Enter in a settings field, and the Home signal
chart sizing) also apply to other boards; everything else in upstream WadaMesh is
unchanged.

## Build & flash

Install [PlatformIO](https://platformio.org/), clone this repository, then build or upload
the Attaky environment:

```bash
git clone -b beta_47-A001 https://github.com/Attaky-Module/wadamesh.git
cd wadamesh
pio run -e attaky_mesh_series_companion_radio_touch
pio run -e attaky_mesh_series_companion_radio_touch -t upload --upload-port <port>
```

Or install it from the Attaky firmware store in your browser.

## Support

- **Attaky Mesh series / this fork** — Attaky [Discord](https://discord.attaky.com)
- **WadaMesh upstream firmware** — file at
  [`ALLFATHER-BV/wadamesh`](https://github.com/ALLFATHER-BV/wadamesh/issues)

---

# ↓ Upstream WadaMesh README ↓

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/wadamesh-readme-dark.svg">
    <img alt="WADAMESH" src="assets/wadamesh-readme-light.svg" width="440">
  </picture>
</p>

<p align="center"><b>A real touchscreen UI for your mesh radio.</b> &middot; open source &middot; GPL-3.0</p>

Touch-UI [MeshCore](https://github.com/meshcore-dev/MeshCore) companion-radio
firmware for the **LilyGo T-Deck / T-Deck Plus** and **Heltec V4 + TFT**
(ESP32-S3).

An LVGL touch UI — map, chat, contacts, channels, settings — split out of
[meshcomod](https://github.com/ALLFATHER-BV/meshcomod). The app depends on a
MeshCore fork via PlatformIO `lib_deps`.

## Boards

See **[DEVICES.md](DEVICES.md)** for the full support matrix, install paths and
per-board status.

- LilyGo T-Deck / T-Deck Plus — env `LilyGo_TDeck_companion_radio_touch` (stable)
- Heltec V4 + TFT + CHSC6x touch — env `heltec_v4_tft_companion_radio_usb_tcp_touch` (stable)
- Tanmatsu (ESP32-P4) — built from `tanmatsu/` (ESP-IDF), ships via the Tanmatsu app store
- Elecrow ThinkNode M9 — env `ThinkNode_M9_companion_radio_touch` (beta)
- RAK WisMesh Tap V2 (RAK3312) — env `rak_tap_v2_companion_radio_touch` (beta)

## Architecture

This repo holds only the **app**: the `companion_radio` glue, the `ui-touch`
LVGL UI, the two boards' glue/variants, and `platformio.ini`. The **MeshCore
core is not vendored here** — it's pulled as a library via `lib_deps` from the
[`ALLFATHER-BV/meshcomod`](https://github.com/ALLFATHER-BV/meshcomod) monorepo
(the same repo as the non-touch firmware), pinned by a lean source-only `core-*`
git tag. The touch-app files this repo owns (TouchPrefsStore, WifiRuntimeStore,
the transports, …) are dropped from the lib via `-DMC_VENDORED_TOUCH_APP` so they
aren't compiled twice. The build is byte-identical to the original in-tree
meshcomod firmware.

## Build

[PlatformIO](https://platformio.org/) pulls the core fork and all libraries
automatically:

```bash
pio run -e heltec_v4_tft_companion_radio_usb_tcp_touch   # Heltec V4 TFT
pio run -e LilyGo_TDeck_companion_radio_touch            # LilyGo T-Deck
# or just `pio run` to build both
```

Flash with the NVS-preserving 4-component chain (bootloader / partitions /
boot_app0 / firmware at `0x0 / 0x8000 / 0xe000 / 0x10000`) so saved Wi-Fi
credentials survive — not a merged image, which 0xFF-pads and wipes NVS.

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md). One topic per
PR; inbound contributions are accepted under the project's GPL-3.0 license.

## License

**GPL-3.0-or-later** — see [LICENSE](LICENSE). wadamesh is copyleft: anyone who
distributes a build or a fork must also make their source available under the GPL.
This keeps the UI open and concentrates community effort instead of fragmenting it
into closed forks.

wadamesh incorporates and depends on
[MeshCore](https://github.com/meshcore-dev/MeshCore) (MIT, © Scott Powell /
rippleradios.com) and other third-party components — see [NOTICE](NOTICE) for the
full list and their licenses. MeshCore-derived files keep their MIT notices; the
combined work is distributed under the GPL (MIT is GPL-compatible). The MeshCore
fork that wadamesh builds against stays **MIT** on purpose, so its Wi-Fi/BLE hooks
remain upstreamable to MeshCore.
