# t7-release

A minimal internal DLL for Call of Duty: Black Ops III (T7), loaded as a `d3d11.dll`
proxy. The focus is protection and quality-of-life: it hardens the client against the
lobby/network crash exploits that plague BO3 and smooths out the worst of the engine's
rough edges. Zero external dependencies, including a from-scratch x64 inline-hook engine.

## Features

Every string in the binary is RC4-encrypted at compile time (per-string unique key), so
the strings view of a dumped build is empty.

**Crash / exploit guards**

- `demonware` — drops malicious demonware + Steam-P2P instant messages (remote popup / remote cbuf).
- `oob` — filters connectionless out-of-band commands (rcon, mstart, relay, ...).
- `lobbymsg` — bounds-checks and validates lobby message deserialization (array overflow, spoofed sender, voice-packet OOB, forged kicks).
- `netchan` — gamestate + reliable-ack + packet-spoof guards on the netchannel layer.
- `markup` — defuses the `^`-code text parser crash surface.
- `mspreload` — blocks the forced side-load (`mspreload` / `msload`) crash.
- `callvote` — clamps and sanitizes oversized vote strings.
- `presence` — clamps the LivePresence player count to stop the serialize overflow.
- `paragon` — guards the malformed paragon-icon scoreboard lookup.

**Privacy**

- `infoleak` — blocks raw-UDP to non-host endpoints and nulls Steam rich presence, so your IP isn't handed out to the lobby.
- `presence` — hides your rich presence from non-friends.

**Quality of life**

- `steamqol` — caches DLC/app install queries and throttles the friend-list rebuild, killing the Steam-IPC menu FPS hitches.
- `antiquit` — clears the forced menu-block branch.
- `logo` — animated frontend banner from an embedded resource.

**Unlocks**

- `inventory` — reports owned quantities / purchased slots for cosmetic items.

## Hotkeys

| Key | Action |
| --- | --- |
| F1  | Toggle disable cutscenes (while in a game; menu backgrounds are unaffected) |
| F2  | Disconnect from the current session |
| F7  | Toggle raw-UDP block (breaks dedi / direct connect while on) |
| F8  | Toggle Steam-P2P block (raw-UDP dedis still work while on) |

## Build

Requires Visual Studio with the C++ x64 toolchain.

```
build.bat
```

The script sets up `vcvars64`, generates the proxy export forwarders, compiles Detours,
compiles the DLL, and writes `build\bin\t7-release.dll`. On success it auto-deploys to
your Black Ops III install (located via the Steam registry) as `d3d11.dll`.

## Install

Drop the built `d3d11.dll` next to `BlackOps3.exe` in your game directory. The game loads
it from its own folder before the system copy; the proxy forwards every real `d3d11`
export on to `C:\Windows\System32\d3d11.dll`, so rendering is unaffected.

## Layout

```
src/
  dllmain.cpp        entry point + init/tick loop
  engine/            game offsets and typed wrappers
  patches/           one folder per feature module
  features/          non-patch features (logo)
  utils/             crypt (string encryption), hook (x64 inline-hook engine), log, mem, resource, exceptions
  proxy/             generated d3d11 export forwarders
tools/               gen_proxy.ps1, deploy.ps1
data/                embedded resources + crc patch blobs
```

## Hook engine

`utils/hook` is a self-contained x64 inline hooker — no third-party library. It length-
decodes the target prologue, relocates the stolen bytes to a trampoline allocated within
±2 GB (fixing RIP-relative operands and rel32 branches), and installs a 14-byte absolute
jmp. `attach(&original, detour)` redirects the function and rewrites `original` to point
at the trampoline; `detach(target)` restores the original bytes.
