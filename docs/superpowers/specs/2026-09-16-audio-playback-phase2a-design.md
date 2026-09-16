# Audio Playback — Phase 2a (WAV) Design

Date: 2026-09-16
Status: Approved for implementation
Parent spec: `docs/superpowers/specs/2026-09-15-sd-audio-player-continuation-design.md` §5 (Phase 2 — Audio playback)

## 1. Context

Phase 1 (merged: commit `83e41db`) gave the project a working `listDir()` for
browsing the SD card and fixed several build-blocking bugs, but no audio
ever plays — `FPlayer`/`I2SAudio` are unfinished skeletons and nothing in
`main.cpp` calls into them.

Library research done during brainstorming (see chat history; the actual
`AudioPlayer`/`AudioSourceSDFAT` headers could not be fetched from GitHub —
404s on the expected paths, sparse Doxygen coverage — so several details
below are **best-effort from wiki pages, class-reference pages, and
community discussion threads, not the real header**) found:

- `AudioPlayer` do have a default constructor, but a default-constructed
  one has no source/decoder/output wired — the real usage is
  `AudioPlayer(AudioSource&, AudioOutput&/AudioStream&, AudioDecoder&)`.
- The library ships `AudioSourceSDFAT<AudioFs, AudioFile>` (template on
  filesystem/file types, matching this project's own `sd_t`/`file_t`
  typedefs) with constructor
  `AudioSourceSDFAT(const char* startFilePath = "/", const char* ext = ".mp3", int chipSelect = PIN_CS, int speedMHz = 2)`.
  It does its own directory traversal/filtering internally
  (`nextStream`/`previousStream`/`selectStream`), independent of this
  project's `listDir()`.
- `AudioPlayer::setPath(const char* path)` exists and selects a file by
  explicit path, separately from the source's own next/previous
  traversal.
- `MultiDecoder` lets one `AudioPlayer` handle multiple formats (MP3, WAV,
  ...) by mime-type-based dispatch to registered sub-decoders — needed
  eventually for WAV+MP3, but out of scope for this sub-phase (2a).

## 2. Goal (this sub-phase: 2a)

Get **WAV playback working end-to-end** on real hardware: select a `.wav`
file from the SD card's root directory (using Phase 1's `listDir()`
results) and hear it play over I2S, with `next`/`previous`/volume control
exercised via Serial commands (no physical buttons yet — that's Phase 3).

MP3 support is **Phase 2b**, deliberately deferred: the exact behavior of
`AudioSourceSDFAT`'s single `ext` constructor parameter (single extension
vs. a pattern supporting multiple) could not be confirmed from available
documentation, and `MultiDecoder`'s mime-type wiring adds real surface
area. Landing WAV-only first, on real hardware, lets Phase 2b start from a
build that has actually compiled once, with the real vendored library
headers available to read (see §7).

## 3. Architecture

**Revision note (ponytail pass):** an earlier draft of this section wrapped
everything in a composition-based `FPlayer` class holding its own
`DirEntry[]` cache and re-implementing next/previous/extension-filtering
by hand. That's deleted. `AudioPlayer` already exposes
`begin()`/`next()`/`previous()`/`setVolume()`/`volume()`/`copy()` as
public methods, and `AudioSourceSDFAT`'s own `ext` constructor parameter
already does single-extension filtering — for a WAV-only sub-phase, a
wrapper class around those would just be a same-signature pass-through
with no added behavior. The custom `listDir()`-to-player bridge is
deferred to Phase 3, when the UI actually has something to call it with;
building it now, with no caller, was scaffolding for a consumer that
doesn't exist yet.

**No wrapper class — plain composition via file-scope globals**, matching
every real usage example of this library (they never subclass or wrap
`AudioPlayer`). `lib/Audio/fplayer.h`/`fplayer.cpp` keeps its existing
location and role (this is where the project already puts audio-related
code) but now just declares/defines the global objects plus one setup
function:

```cpp
// fplayer.h
extern AudioPlayer player;
void beginAudioPlayer();
```

`fplayer.cpp` owns the actual `AudioSourceSDFAT`, `WAVDecoder`, and
`I2SStream` objects (as file-local statics — nothing outside this file
needs them directly, only `player` does) and constructs `player` from
them at file scope. `beginAudioPlayer()` does the one piece of real,
non-redundant setup work: building the `I2SConfig` with this project's
pins and calling `i2s.begin(cfg)`, then `player.setVolume()` /
`player.begin()`. `main.cpp` calls `beginAudioPlayer()` once from
`setup()` and drives everything else directly through the global
`player` object — there is nothing left for a wrapper class to add.

The old `I2SAudio` wrapper class (`lib/Audio/i2s_audio_init.{h,cpp}`) is
**deleted**, same reasoning as above one layer down: its only job —
deferring `i2s.begin(cfg)` until an explicit call — has no value once
`I2SStream` is used directly as `AudioPlayer`'s output. Its pin constants
(`PIN_MCLK`, `PIN_WCLK`, `PIN_DOUT`, `PIN_BCLK`, `MCLK_MULTI`) move into
`fplayer.h`.

## 4. Pin reassignment

`PIN_DOUT` (was GP4) and `PIN_MCLK` (was GP5) collide with
`SD_MISO_PIN`/`SD_CS_PIN` in `lib/File/file_sdfat.h`. This sub-phase is the
first time the firmware actually calls `i2s.begin()` while the SD card is
mounted, so the collision stops being theoretical. New assignment:

| Signal | Old pin | New pin |
|---|---|---|
| `PIN_BCLK` | GP2 | GP2 (unchanged) |
| `PIN_WCLK` | GP3 | GP3 (unchanged) |
| `PIN_DOUT` | GP4 | **GP8** |
| `PIN_MCLK` | GP5 | **GP9** |

GP8/GP9 don't collide with SD (GP4-7), the release build's SPI OLED
(GP10-14), the Wokwi build's I2C OLED (GP12-13), or the serial monitor
wiring (GP0-1) in `diagram.json`. **You must re-check your physical
wiring against these new numbers** before flashing real hardware — this
sub-phase only fixes the code-level collision, it doesn't know your
actual DAC wiring.

## 5. Navigation model

`AudioSourceSDFAT` is constructed with `ext = ".wav"`, so its own
`nextStream()`/`previousStream()` traversal (reached via
`player.next()`/`player.previous()`) already does exactly the filtering
this sub-phase needs — no custom directory-walking code required. `next`,
`previous`, `begin(index)`, `setVolume`, `volume`, and `copy` are all
called directly on the global `player` object; nothing in this project
wraps them.

Phase 1's `listDir()` is **not used by playback in this sub-phase** — it
stays exactly what Phase 1 built it for (the SD-browsing demo already in
`main.cpp`). Bridging "the file the UI highlights" to "the file that
plays" is Phase 3's concern, once there's an actual UI selection to
bridge from; `AudioPlayer::setPath()` is confirmed to exist for that when
the time comes.

`copy()` must be called every `loop()` iteration for playback to actually
progress — this is how the library's pull-driven model works: nothing
plays unless something calls `copy()` regularly.

## 6. Wiring in `main.cpp`

`setup()` gains, after `initCard()` succeeds:

```cpp
beginAudioPlayer();   // builds I2SConfig with this project's pins, i2s.begin(cfg), player.setVolume(0.5f), player.begin()
```

`loop()` becomes:

```cpp
void loop() {
  player.copy();
  if (Serial1.available()) {
    char c = Serial1.read();
    if (c == 'n') player.next();
    else if (c == 'p') player.previous();
    else if (c == '+') player.setVolume(min(player.volume() + 0.1f, 1.0f));
    else if (c == '-') player.setVolume(max(player.volume() - 0.1f, 0.0f));
  }
}
```

`player` (the global `AudioPlayer` declared in `fplayer.h`) has file-scope
lifetime spanning `setup()` and `loop()`, matching every real usage
example of this library.

## 7. Known unknowns — verify against the real vendored header first

Because the actual `AudioSourceSDFAT.h`/`AudioPlayer.h` source could not be
fetched (GitHub 404s on the expected paths; Doxygen coverage for this
specific class is thin), this design is built from wiki pages, a
class-reference page, and community-example threads. **The first
implementation step must be: once `pio run` has fetched
`arduino-audio-tools` into `.pio/libdeps/`, read the actual installed
`AudioSourceSDFAT.h` and `AudioPlayer.h` and reconcile this section
against them before writing the rest of `fplayer.cpp`.** Specifically:

- Whether `AudioSourceSDFAT` opens its own independent `SdFat`-family
  instance internally (separate from this project's global `sd`/`root` in
  `file_sdfat.cpp`), and whether that's safe to run alongside our own
  already-`begin()`'d SD access on the same SPI bus/CS pin. If the library
  re-initializes SPI/CS on each of its own operations (which is the
  common pattern for this kind of wrapper), sequential access from two
  independent objects should work but is wasteful — acceptable for 2a,
  worth revisiting only if it actually breaks on hardware.
- The exact constructor call this project needs:
  `AudioSourceSDFAT<sd_t, file_t> source("/", ".wav", SD_CS_PIN, <speed>)`
  — confirm parameter order and types against the real header before
  transcribing into `fplayer.cpp`. Single-extension filtering (what 2a
  actually needs) is the one documented, confirmed-from-a-real-example
  usage of this constructor, so this specific call is lower-risk than the
  multi-format question Phase 2b will have to resolve.

If any of these turn out materially different from this section's
assumptions, the implementer documents the deviation (in the fix report,
or as a `knowledge://` note via privmind) and proceeds with the real API
rather than forcing the assumption.

## 8. Error handling

- `listDir()` returns 0 (empty directory): `begin()`/`next()`/`previous()`
  are no-ops, `currentName()` returns an empty/placeholder string.
- `player.setPath()` or `player.play()` fails (bad file, decoder can't
  open it, I2S not ready): the failure propagates as a `false` return
  from `FPlayer`'s method; `main.cpp` logs it to `Serial1` rather than
  silently continuing.
- `begin(index)` called with an out-of-range or non-playable index:
  returns `false`, no state change.

## 9. Testing

No automated test suite exists in this project (documented in Phase 1's
plan; still true). Verification for 2a:

- `pio run -e release` and `pio run -e wokwi` compile cleanly — this is
  the first real compile of anything in `lib/Audio/`, since Phase 1's
  final review confirmed `lib/Audio` had never actually been reachable
  from the build graph before FPlayer becomes wired into `main.cpp`.
- Real hardware test (Wokwi has no meaningful way to verify actual audio
  output): flash `release`, put a couple of short `.wav` files at SD root,
  confirm `next`/`previous` over Serial move between them audibly, volume
  `+`/`-` audibly changes level, and `Serial1` prints match what's
  actually playing.

## 10. Out of scope (this sub-phase)

- MP3 decoding (`MultiDecoder`, `MP3DecoderHelix`) — Phase 2b, once this
  compiles and plays on real hardware and the real `AudioSourceSDFAT`
  header has been read.
- DSF/DSD — still out of scope per the parent spec.
- Physical buttons — Phase 3.
- Subdirectory navigation, wraparound next/previous, volume persistence,
  playlists — all still out of scope per the parent spec's §7.
