[![Downloads](https://img.shields.io/github/downloads/tokenizestring/t7-release/total?style=for-the-badge&label=downloads)](https://github.com/tokenizestring/t7-release/releases)

# t7-release

A small internal DLL for Call of Duty: Black Ops III (T7), loaded as a `d3d11.dll` proxy. It's built around two things: protection and quality of life. It hardens the client against the lobby and network crash exploits that BO3 is riddled with, and it files down a lot of the engine's rough edges along the way. There are no external dependencies at all, right down to a from-scratch x64 inline hook engine.

## About the name

"T7" is the community and internal codename for Black Ops III. It stands for Treyarch 7, as in Treyarch's seventh Call of Duty. It's just shorthand for the game, nothing more.

This project ("t7-release") has nothing to do with "T7 Patch" or any other similarly named tool. Different author, different codebase, different goals. The shared "t7" only refers to the same game, so please don't mix the two up.

## Verified

This runs on a live Black Ops III process. Every module initializes and every inline hook lands cleanly (no `undecodable prologue` failures), all placed by the in-house x64 hook engine. The `oob(sv): connect` line near the bottom of the log is the guard filtering real inbound connection traffic as it comes in.

![t7-release startup log](docs/startup-log.png)

## Features

Every string in the binary is RC4-encrypted at compile time with a unique per-string key, so the strings view of a dumped build comes up empty.

Most of what follows is the same idea: a hook sits on a specific attack surface, checks the incoming data, and drops or clamps anything malformed before the game ever sees it. When a guard blocks something, a small toast pops up on screen so you know it happened.

### In-game menu

Press Insert to open it. It's a hand-drawn D3D11 overlay (no ImGui) with tabs for Protection, Settings, About and Exit. Every protection listed below is a toggle and they all default to on, so if one ever gets in your way you can flip it off without rebuilding anything. Arrow keys move, Enter toggles or runs an item, Insert closes.

### Server-command guards (`servercmd`)

Every host to client server command is validated before the game processes it, and logged so you can review what a host sent:

- `/` command: clamps the entity index. The game didn't bound it at all, so an out-of-range index read a pointer out of bounds and then wrote through it, which is straight up remote code execution.
- `7` command: clamps the team and entity indices to stop a remote out-of-bounds write.
- message commands (`)` `<` `;` `O`): routes the formatter through a large scratch buffer so a `[{...}]`-token string can't smash the stack.
- `D` command: bounds the arg count and rate-limits the opcode. It sets a LUI data model and notifies every subscriber, so a host spamming it drains the LUI element pool and crashes you (`Failed to allocate from element pool`).
- kick immunity: neutralizes the host-triggered forced disconnects. That covers the bad configstring index, the 64 KB configstring-pool overflow, the big-configstring reassembly overflow, the reliable-sequence cycle-out, and the BG-cache checksum mismatch (configstring 3241).

### Client-command guards (`clientcmd`)

The other direction. When you host, connected clients can send you commands, and a few of them are dangerous. This logs every one and shuts down the two that actually hurt:

- `mrp`: clamps the menu-response index. It was fed straight into pointer math with no bounds check, so a client sending a large index walked a wild pointer off into memory and crashed the host.
- `video_start` / `video_start_looped`: dropped when they come from a remote client. Left alone, any client could black out your screen with a fullscreen video, and worse, feed it an `http://` URL and make your machine fetch from an address they control.

### Crash and exploit guards

- `workshop`: drops host-forced Steam Workshop UGC, which kills the phantom-DLL sideload RCE (a mod that ships `RzChromaSDK64.dll` or `atiadlxx.dll` and loads it onto everyone in the lobby).
- `demonware`: drops malicious demonware and Steam-P2P instant messages (remote popup and remote cbuf).
- `oob`: filters connectionless out-of-band commands (rcon, mstart, relay, and so on).
- `lobbymsg`: bounds-checks and validates lobby message deserialization, covering array overflow, spoofed sender, voice-packet OOB and forged kicks.
- `netchan`: gamestate, reliable-ack and packet-spoof guards on the netchannel layer.
- `markup`: defuses the `^`-code text parser crash surface.
- `mspreload`: blocks the forced side-load (`mspreload` / `msload`) crash.
- `callvote`: clamps and sanitizes oversized vote strings.
- `presence`: clamps the LivePresence player count so the serializer can't overflow.
- `paragon`: guards the malformed paragon-icon scoreboard lookup.
- `notetrack`: guards the dangling xanim notetrack lookup so a stale entry pointer returns a safe sentinel instead of being dereferenced.

### Privacy

- `infoleak`: blocks raw-UDP to non-host endpoints and nulls Steam rich presence, so your IP doesn't get handed out to the lobby.
- `presence`: hides your rich presence from non-friends.
- `video`: blocks host-forced `http://` and `https://` video loads, which are both an IP-leak deanon and a remote video-decoder attack surface.

### Quality of life

- `movement`: optional fps-safe movement. High frame rates break BO3's movement, wall-running drops and you get snagged on invisible barriers, because the physics runs on a tiny per-frame timestep. This simulates your own player on a fixed 125 Hz tick instead, so movement feels the same at any frame rate while your render fps stays uncapped. It's off by default. Turn it on under Settings and test it live, since it touches the movement path.
- `video`: F1 skips in-game cutscenes. It hooks the cinematic loader so they never start in the first place, and menu backgrounds are left alone.
- `steamqol`: caches DLC and app install queries and throttles the friend-list rebuild, which kills the Steam-IPC menu fps hitches.
- `perf`: removes the hidden loading-screen fps cap. Stock BO3 throttles the renderer hard while loading, so load screens crawl and stutter. With the cap gone they run smooth and fully uncapped.
- `texstream`: raises the per-frame texture streaming budget so textures pop in faster instead of staying blurry after a load.
- `lod`: disables the aggressive auto-LOD-cull so nearby props and barriers stop vanishing when you back away from them.
- `antiquit`: clears the forced menu-block branch.
- `logo`: animated frontend banner from an embedded resource.

### Unlocks

- `inventory`: reports owned quantities and purchased slots for cosmetic items.

## Server browser

A live browser for dedicated Black Ops III servers, built into the Insert menu. It searches demonware for dedicated sessions and lists them paginated, filtering out empty and full lobbies. Each row shows the address, live player count and session type, and the cards auto-scale to their contents.

Selecting a server dispatches a **probe**: a two-step join handshake (join request, then a member-info reply carrying our serialized address) that pulls the host's full player roster back without ever connecting. The roster shows up in its own card beside the server, with each player's name, clan tag and client number.

- **Auto probe** (menu toggle): probes every listed dedi once a second, skipping full lobbies, and keeps running with the menu closed. A green dot on a row means our probe is still live in that server.
- **Steam lobby join**: for any probed dedi it also joins the underlying Steam lobby, so you can read the in-lobby chat. With auto probe on this happens automatically for every server, and you can sit in any number of them at once. Steam chat can be sent back out as well.

### Hosting (`matchmaking`)

Create and advertise your own session so it shows up in the browser and clients can find and join you. The host-side join handlers (join request and member info) are replaced with hardened versions at the same time.

### Recently seen (`recents`)

Keeps a rolling record of players seen across sessions: xuid, name, when they were last seen, and a short history of the IP and port they connected from.

### Network test harness (`send`)

An internal harness for firing a specific packet at a target address (relay probe, out-of-band command, oversized join, host disconnect, host migrate, nested print) to exercise the guards above against a real endpoint.

## Hotkeys

| Key | Action |
| --- | --- |
| Insert | Open or close the menu |
| F1  | Toggle cutscene skipping (while in a game; menu backgrounds are unaffected) |
| F2  | Disconnect from the current session |
| F7  | Toggle raw-UDP block (breaks dedi and direct connect while on) |
| F8  | Toggle Steam-P2P block (raw-UDP dedis still work while on) |

Everything on the hotkeys and more is also in the Insert menu.

## Build

You need Visual Studio with the C++ x64 toolchain.

```
build.bat
```

The script sets up `vcvars64`, generates the proxy export forwarders, compiles the DLL with the in-house hook engine (no third-party libraries), and writes `build\bin\t7-release.dll`. If it builds, it auto-deploys to your Black Ops III install (found through the Steam registry) as `d3d11.dll`.

## Install

Drop the built `d3d11.dll` next to `BlackOps3.exe` in your game folder. The game loads it from its own directory before the system copy, and the proxy forwards every real `d3d11` export on to `C:\Windows\System32\d3d11.dll`, so rendering is unaffected.

At runtime it writes to a `t7-rework` folder next to the game:

```
t7-rework/
  logs/            t7-release.log (startup and per-frame activity)
  exceptions/      exception-<date>.txt (one file per fault)
```

## Reporting crashes

If the DLL faults it drops a full dump into `t7-rework/exceptions/exception-<date>.txt` with the code, faulting address, registers and a stack walk. Please post that file in the Discord so it can be looked at and fixed: https://discord.gg/WWKZsCCesT

## Layout

```
src/
  dllmain.cpp        entry point plus the init and tick loop
  engine/            game offsets and typed wrappers
  patches/           one folder per feature module
  features/          non-patch features (overlay, logo, server browser, recents, send)
  menu/              the in-game overlay menu
  utils/             crypt (string encryption), hook (x64 inline-hook engine), log, mem, resource, exceptions
  proxy/             generated d3d11 export forwarders
tools/               gen_proxy.ps1, deploy.ps1
data/                embedded resources and crc patch blobs
```

## Hook engine

`utils/hook` is a self-contained x64 inline hooker with no third-party library behind it. It length-decodes the target prologue, relocates the stolen bytes to a trampoline allocated within 2 GB (fixing RIP-relative operands and rel32 branches on the way), and installs a 14-byte absolute jmp. `attach(&original, detour)` redirects the function and rewrites `original` to point at the trampoline, and `detach(target)` puts the original bytes back.
