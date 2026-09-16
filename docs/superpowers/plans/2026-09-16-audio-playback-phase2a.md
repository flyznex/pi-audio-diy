# Audio Playback Phase 2a (WAV) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Get `.wav` playback from the SD card root working over I2S, controllable via Serial commands (no physical buttons yet).

**Architecture:** No wrapper class. `lib/Audio/fplayer.h`/`fplayer.cpp` declares/constructs a global `AudioPlayer player` wired from a file-local `AudioSourceSDFAT<sd_t,file_t>` (constructed with `ext=".wav"`, doing its own file traversal/filtering — no custom directory-walking code needed), a `WAVDecoder`, and an `I2SStream`, plus one `beginAudioPlayer()` setup function. `main.cpp` calls `beginAudioPlayer()` once and drives everything else directly through `player` — `AudioPlayer` already exposes `begin`/`next`/`previous`/`setVolume`/`volume`/`copy` as public methods, so there's nothing for a wrapper to add. (An earlier draft wrapped this in a composition-based `FPlayer` class with its own file-listing cache and bridged Phase 1's `listDir()`; deleted as a same-signature pass-through with no consumer yet — see spec §3.) The old `I2SAudio` wrapper is deleted; its pin constants move into `fplayer.h`, two of them renumbered to resolve a real GPIO collision with the SD card pins.

**Tech Stack:** PlatformIO, Arduino framework on RP2040, `arduino-audio-tools` (pschatzmann), SdFat. Same no-test-framework situation as Phase 1 — verification is compile-clean + real hardware audio test.

**Spec:** `docs/superpowers/specs/2026-09-16-audio-playback-phase2a-design.md`

## Global Constraints

- No wrapper class around `AudioPlayer` — use the global `player` object directly wherever its public methods already cover what's needed (spec §3).
- WAV only — no MP3, no `MultiDecoder`, no DSF in this plan (spec §2, §10).
- I2S pins: `PIN_BCLK=2`, `PIN_WCLK=3` (unchanged), `PIN_DOUT=8`, `PIN_MCLK=9` (moved off GP4/GP5 — those collide with `SD_MISO_PIN`/`SD_CS_PIN` in `lib/File/file_sdfat.h`) (spec §4).
- Phase 1's `listDir()` is not used by playback in this plan — stays exactly what Phase 1 built it for (the SD-browsing demo already in `main.cpp`) (spec §5).
- No physical buttons yet — control is via single-character commands read from `Serial1` (spec §6).
- **This is the first time anything under `lib/Audio/` has ever been reachable from the PlatformIO build graph** (Phase 1's final review confirmed `main.cpp` never included `fplayer.h` before). Nothing in this plan's code has been compiled by a real compiler. Per spec §7, the exact signature of `AudioSourceSDFAT`'s constructor is best-effort from documentation and a real community example, not a read of the real vendored header — flag this prominently rather than treating a clean-looking diff as proof it compiles.
- Both `pio run -e release` and `pio run -e wokwi` are expected to build cleanly, but — as in Phase 1 — this sandbox has no `pio` installed; treat any build-verification step the same way Phase 1 did (attempt it, and if unavailable, self-check by hand and say so plainly).

---

### Task 1: Set up the global `AudioPlayer` in `lib/Audio/`, delete `I2SAudio`, enable `USE_SDFAT`

**Files:**
- Modify: `lib/Audio/fplayer.h` (full rewrite)
- Modify: `lib/Audio/fplayer.cpp` (full rewrite)
- Delete: `lib/Audio/i2s_audio_init.h`
- Delete: `lib/Audio/i2s_audio_init.cpp`
- Modify: `platformio.ini` (add `-D USE_SDFAT` to both `[env:release]` and `[env:wokwi]` `build_flags`)

**Interfaces:**
- Consumes: `sd_t`, `file_t`, `SD_CS_PIN` from `lib/File/file_sdfat.h` (Phase 1, already in the tree). Does **not** consume `listDir()`/`DirEntry` — playback doesn't use them in this plan.
- Produces (used by Task 2):
  ```cpp
  extern AudioPlayer player;
  void beginAudioPlayer();
  ```

- [ ] **Step 1: Delete the old `I2SAudio` wrapper**

```bash
git rm lib/Audio/i2s_audio_init.h lib/Audio/i2s_audio_init.cpp
```

- [ ] **Step 2: Replace `lib/Audio/fplayer.h` with the new declarations**

Replace the full file contents with:

```cpp
#pragma once

#include "AudioTools.h"
#include "AudioTools/AudioPlayer.h"
#include "file_sdfat.h"

#ifdef USE_SDFAT
#include "AudioLibs/AudioSourceSDFAT.h"
#endif

#define AUDIOINFO_DEFAULT AudioInfo(44100, 2, 16)
const int PIN_BCLK = 2;   // BCCLK bit-clock
const int PIN_WCLK = 3;   // LRCLK left-right clock
const int PIN_DOUT = 8;   // Data in/out (was GP4 — collided with SD_MISO_PIN)
const int PIN_MCLK = 9;   // Master-clock (was GP5 — collided with SD_CS_PIN)
const int MCLK_MULTI = 512;

extern AudioPlayer player;

// Configures I2S with this project's pins and starts playback from
// the SD card root. Call once from setup(), after initCard() succeeds.
void beginAudioPlayer();
```

- [ ] **Step 3: Replace `lib/Audio/fplayer.cpp` with the new implementation**

Replace the full file contents with:

```cpp
#include "fplayer.h"

static AudioSourceSDFAT<sd_t, file_t> source("/", ".wav", SD_CS_PIN, 4);
static WAVDecoder decoder;
static I2SStream i2s;
AudioPlayer player(source, i2s, decoder);

void beginAudioPlayer()
{
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.copyFrom(AUDIOINFO_DEFAULT);
    cfg.pin_bck = PIN_BCLK;
    cfg.pin_ws = PIN_WCLK;
    cfg.pin_data = PIN_DOUT;
    cfg.pin_mck = PIN_MCLK;
    cfg.mck_multiplier = MCLK_MULTI;
    i2s.begin(cfg);

    player.setVolume(0.5f);
    player.begin();
}
```

- [ ] **Step 4: Add `-D USE_SDFAT` to both PlatformIO environments**

In `platformio.ini`, `[env:release]`'s `build_flags` currently reads:
```ini
build_flags =
	-DOLED_SPI=1
	-D SD_FAT_TYPE=1
```
Add `-D USE_SDFAT` as a third line:
```ini
build_flags =
	-DOLED_SPI=1
	-D SD_FAT_TYPE=1
	-D USE_SDFAT
```
`[env:wokwi]`'s `build_flags` currently reads:
```ini
build_flags =
	-D SD_FAT_TYPE=1
```
Add `-D USE_SDFAT` as a second line:
```ini
build_flags =
	-D SD_FAT_TYPE=1
	-D USE_SDFAT
```

Without this flag, `fplayer.h`'s `#ifdef USE_SDFAT` guard around the `AudioSourceSDFAT` include never fires, and the file fails to compile (`AudioSourceSDFAT` undeclared) — this flag was never set in the original codebase, which is part of why `lib/Audio` was never actually reachable/buildable before.

- [ ] **Step 5: Attempt a build; hand-verify if `pio` is unavailable**

Run: `pio run -e release` and `pio run -e wokwi`.

If `pio` is not installed (as in every prior task on this project), do not block on it. Instead, re-read the full final content of `fplayer.h` and `fplayer.cpp` and confirm by hand: `source`, `decoder`, and `i2s` are fully-defined objects (file-scope `static`, so already constructed) before `AudioPlayer player(source, i2s, decoder);` runs — order in the file matters here, since these are plain globals, not class members; no removed/renamed identifier from the deleted `I2SAudio` (`I2SAudio`, `getStream()`) is referenced anywhere in the tree.

This build is expected to be the first real test of whether the `AudioSourceSDFAT`/`AudioPlayer` signatures assumed here match the actual vendored library (spec §7) — if you have `pio` available and it fails with an unexpected signature/include-path error, that is expected and valuable information, not a sign this step was done wrong; note it in your report rather than trying to guess-fix it blindly.

- [ ] **Step 6: Commit**

```bash
git add lib/Audio/fplayer.h lib/Audio/fplayer.cpp platformio.ini
git commit -m "refactor(audio): wire a global AudioPlayer for WAV playback, drop I2SAudio

fplayer.{h,cpp} now just constructs the library's own AudioSourceSDFAT
+ WAVDecoder + I2SStream + AudioPlayer as file-scope globals and
exposes beginAudioPlayer() — no wrapper class, since AudioPlayer
already exposes begin/next/previous/setVolume/volume/copy directly and
AudioSourceSDFAT's own ext=\".wav\" filter already does the file
traversal this needs. I2SAudio is deleted — I2SStream is used
directly, matching how the library's own examples wire an
AudioPlayer. Moves PIN_DOUT/PIN_MCLK off GP4/GP5 (collided with the SD
card's pins) to GP8/GP9. Enables USE_SDFAT so the AudioSourceSDFAT
include actually fires (it never did before)."
```

---

### Task 2: Wire the player into `main.cpp` with a Serial command harness

**Files:**
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: `extern AudioPlayer player;` and `void beginAudioPlayer();` from Task 1 (`lib/Audio/fplayer.h`).
- Produces: nothing consumed by later tasks in this plan.

- [ ] **Step 1: Add the include**

At the top of `src/main.cpp`, alongside the existing includes:
```cpp
#include "fplayer.h"
```

- [ ] **Step 2: Start playback after the existing SD-listing block in `setup()`**

After the existing block that ends with:
```cpp
  tex.clear();
  if (count > 0) {
    tex.setText(entries[0].name);
    tex.draw();
  }
```
add:
```cpp
  beginAudioPlayer();
```

- [ ] **Step 3: Replace `loop()` with the Serial command harness**

Replace the current (empty) `loop()`:
```cpp
void loop() {
  
}
```
with:
```cpp
void loop() {
  player.copy();
  if (Serial1.available()) {
    char c = Serial1.read();
    if (c == 'n') {
      player.next();
    } else if (c == 'p') {
      player.previous();
    } else if (c == '+') {
      player.setVolume(min(player.volume() + 0.1f, 1.0f));
    } else if (c == '-') {
      player.setVolume(max(player.volume() - 0.1f, 0.0f));
    }
  }
}
```
(`min`/`max` are provided by the Arduino core. `AudioPlayer` doesn't expose
a "current filename" getter in the documented API, so this harness
doesn't try to echo one — you'll confirm track changes by ear during
Task 3's hardware test.)

- [ ] **Step 4: Attempt a build; hand-verify if `pio` is unavailable**

Run: `pio run -e release` and `pio run -e wokwi`.

If unavailable, re-read the full final `src/main.cpp` and confirm: every identifier used in the new code (`player`, `beginAudioPlayer`, `Serial1`, `min`, `max`) is in scope; the new `setup()` call only runs if `initCard()` succeeded (it's after the existing early-`return` on failure, so this is structural, not something this step needs to re-check).

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "feat(main): start audio playback in setup(), add Serial command harness

Calls beginAudioPlayer() after SD init succeeds and adds
single-character Serial1 commands (n/p/+/-) to exercise
next/previous/volume before physical buttons exist (Phase 3)."
```

---

### Task 3: Hand off to the user — real build, reconcile against the vendored library, hardware test

**Files:** none (verification-only task; may produce a follow-up fix commit if the real library API differs from spec §7's assumptions)

**Interfaces:**
- Consumes: the built firmware from Task 2.
- Produces: nothing (terminal task for this plan). If real compilation surfaces a signature mismatch, that becomes a small follow-up fix cycle outside this plan (report the actual compiler error; it will be a fast, targeted fix once the real error text is known).

- [ ] **Step 1: Build for real**

Run `pio run -e release` (and `-e wokwi` if you want the simulator too) in an environment with PlatformIO actually installed. This is the first real compile of everything Task 1 and Task 2 wrote — expect it to need at least one round of fixes.

- [ ] **Step 2: (Superseded — already done in a post-review fix wave, kept for history) Read the real vendored headers before guessing a fix**

A final whole-branch review already did this: it cloned the real upstream `arduino-audio-tools` at tag `v1.2.6` and found two `#include` paths in `lib/Audio/fplayer.h` that didn't exist in that version, plus a real SD/SPI bus-sharing conflict and a macro collision — all fixed in a follow-up commit (see spec §7 for the full account, and `platformio.ini`'s dependency is now pinned to `#v1.2.6` so this is reproducible). If `pio run` still fails for you, the library version may have moved again since — re-run this reconciliation step against whatever version actually gets fetched.

- [ ] **Step 3: Put test files on the SD card and flash**

Copy at least two short `.wav` files to the SD card (root or a subdirectory — `AudioSourceSDFAT`'s traversal is recursive, confirmed against the real header, so it isn't limited to the root directory the way Phase 1's `listDir()` demo is). `WAVDecoder` reads the sample rate/bit depth from each file's own header and reconfigures the I2S output at runtime (confirmed against the real header — `AUDIOINFO_DEFAULT` is only the *initial* config, not a requirement your files must match), so no resampling is needed beforehand. Flash the `release` build to the Pico.

- [ ] **Step 4: Verify over Serial**

Open a serial monitor at 115200 baud on the `Serial1` UART (wired to GP0/GP1 per `diagram.json`, or the physical UART pins on real hardware). You should also see `AudioLogger` output here now (added in the post-review fix wave) — watch for `LOGE`-level lines if nothing plays. Confirm:
- On boot, the first `.wav` file starts playing audibly.
- Sending `n` moves to the next `.wav` file and it's audible.
- Sending `p` moves back — except at the very first track, where it's confirmed (against the real library's index-underflow guard) to replay the same track rather than wrapping to the last one; that's expected library behavior, not a bug to chase.
- Sending `+`/`-` audibly changes volume, clamped at 0.0/1.0.
- If the `release` build errors somewhere in Wi-Fi-related code (`platformio.ini` still lists `Wifi`/`lwIP-Ethernet` in `lib_deps` for what's otherwise a plain, non-`W`, Pico), try adding `-D NO_WIFI` to that environment's `build_flags` — the underlying core's `arduino-pico` framework uses that as an escape hatch to skip `WiFi.h`.

- [ ] **Step 5: Report back**

No commit for this task unless Step 2's reconciliation produced code changes (in which case commit those with a message describing what the real API turned out to be, referencing spec §7). Otherwise, this is a verification checkpoint — note whether Phase 2a is confirmed working before starting Phase 2b (MP3 support).
