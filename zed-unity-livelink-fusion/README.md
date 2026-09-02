# ZED Unity Livelink Fusion — Grimmwelt Märchenwald Fork Notes

This is the C++ sender used for the Grimmwelt Märchenwald installation: it runs the ZED SDK
Fusion module across 2 ZED cameras, does body tracking, and streams the fused skeleton data to
Unity via UDP multicast (`230.0.0.1:20001` by default). For the general upstream documentation of
this tool (setup, Unity-side scripts, etc.), see [../README.md](../README.md).

This file tracks the fork-specific changes and current status, so work can be picked back up
without re-deriving context.

## Current status (2026-09-02)

- **`main`** — stable, includes all fixes below. This is what's been tested and deployed at the
  Grimmwelt site (PC name `Medienstation`).
- **`feature/camera-degraded-unsubscribe`** — one commit ahead of `main`, **not yet tested
  on-site**. See changelog below.
- The tracking watchdog (`trackingwatcher.ps1`) lives in the parent `grimmwelt-maerchenwald` repo
  under `development/it-scripts/`, not in this repo.

## Deployment specifics (verify against the live machine — the repo can drift from it)

- Live launch script: `C:\Users\Medienstation\Documents\PFC\sdk 4.2.0_Cuda12\start_stereo - PERFORMANCE.bat`,
  using calibration `calib_grimmwelt_20251001.json`.
  The repo's own copy of this batch file (under
  `development/projectionstation/3rdparty/sdk 4.2.0_Cuda12/` in the parent repo) points at a
  different, older test calibration (`20251021_buero.json`) — that's expected, it's a
  machine-specific config file, not a bug.
- Deployment PC (as of the last incident): ZED SDK 4.2.5, CUDA 12.9, Nvidia driver 616.56, Windows
  joined to the `ad.grimmwelt.de` domain.
- Logs (all size-bounded/rotated, safe to leave running long-term):
  - Watchdog: `trackingwatcher.log`, next to `trackingwatcher.ps1`.
  - This app's own stdout/stderr — **only captured when launched via the watchdog** (a manual
    double-click of the batch file still shows live console output instead): `fusion_stereo.log` /
    `fusion_stereo.err.log`, next to the batch file.
  - ZED SDK's own internal log (always written, regardless of how the app is launched):
    `C:\ProgramData\Stereolabs\zedlog_*.log`.

## Changelog

### `main` — crash & stability fixes
- **Fixed the original startup crash** (`0xc0000409` in `ucrtbase.dll`, right after camera
  detection / OpenGL window open): an uncaught `SocketException` from
  `UDPSocket::setMulticastTTL()` (Windows `WSAEINVAL` — root cause on this specific PC never
  conclusively identified despite extensive isolation testing, see "Open items" below) was
  propagating out of `main()` and calling `std::terminate()`. UDP setup is now wrapped in
  try/catch; failing to set the TTL explicitly is non-fatal since `1` is already Windows' default
  multicast TTL.
- Set `glewExperimental = GL_TRUE` before `glewInit()` — defensive fix for a well-known
  GLEW/driver crash pattern (ruled out as the actual cause here, but harmless and worth keeping).
- Removed the `NEURAL_LIGHT` depth-mode option — not supported by the installed SDK 4.2.5, so this
  code path had been silently unbuildable on this SDK version.
- `PracticalSocket`: `SocketException` now reports the real Windows error via
  `WSAGetLastError()`/`FormatMessage` instead of a meaningless `errno`-based message on Windows;
  widened the socket descriptor from `int` to `uintptr_t` to correctly match Windows' `SOCKET`
  (`UINT_PTR`) type.
- **In-app camera-health watchdog**: each camera now tracks time since its last successful
  `grab()`. If a camera doesn't recover within 60s (the SDK's own internal reconnect logic was
  observed to give up around ~45s), the app exits cleanly instead of continuing in a silently
  broken state (the OpenGL window stays open either way) — the external process watchdog then
  restarts it. Root cause of the underlying camera dropouts: a genuine USB/hardware connection
  issue. The SDK's own `zedlog` showed repeated `[Grab] Camera module reset`, eventually followed
  by `Cannot start camera stream` / `Failed to recover image capture... Timeout`, then an access
  violation crash inside `sl_zed64.dll` itself — a closed-source SDK robustness bug, not something
  fixable in this codebase.
- Also stopped busy-spinning the per-camera grab thread while a camera is failing to grab (small
  sleep added).

### `feature/camera-degraded-unsubscribe` (untested)
- Addresses a reported symptom: tracking becomes inaccurate when only one of the two cameras is
  effectively working (as opposed to a hard crash). After 25s without a successful grab — well
  before the 60s hard-restart threshold above — the affected camera is proactively unsubscribed
  from the Fusion module (`fusion.unsubscribe()`) instead of staying registered while it delivers
  stale/no data. Rationale: fusing in a half-dead camera's noise likely hurts tracking quality more
  than cleanly falling back to whichever camera(s) are actually working. Automatically
  re-subscribed once the camera resumes grabbing.
- **Needs on-site testing before merging to `main`.**

### Watchdog hardening (in the parent repo, `development/it-scripts/trackingwatcher.ps1`)
- Hang detection (not just crash detection) via window responsiveness.
- Persistent, rotated logging with timestamps.
- Mutex against two watchdog instances running at once, plus cleanup of duplicate app instances
  (both scenarios can otherwise cause two processes to fight over the same 2 cameras).
- Crash-loop backoff: after repeated restarts in a short window, waits longer and logs a clear
  warning instead of hammering a persistently broken camera/hardware.
- Now also captures this app's stdout/stderr to rotated log files, but only for its own
  hidden/unattended launches (see "Deployment specifics" above).

## Open items / known limitations

- The `setsockopt(IP_MULTICAST_TTL)` `WSAEINVAL` on the deployment PC was never conclusively
  root-caused. Ruled out: buffer size/type, socket descriptor width, Winsock init imbalance, race
  conditions, firewall rules, Defender ASR/Network Protection, process name/path association. It's
  non-fatal now (falls back to the OS default TTL of 1), so it hasn't blocked deployment, but the
  "why" remains open if it's ever worth revisiting.
- The underlying USB/camera hardware instability (leading to the SDK-internal crash after ~2-3h of
  uptime) is not fixable in this codebase — mitigated via the camera-health watchdog and the
  external process watchdog, not resolved at the source. If it recurs frequently, worth checking
  on-site: cabling/connector wear, cooling/airflow around the cameras and any USB hub, and that USB
  Selective Suspend is disabled.
- The live batch file, calibration file, and watchdog config path are machine-specific and
  maintained by hand outside version-control parity — always verify the actual files on
  `Medienstation` before assuming the repo reflects production.
