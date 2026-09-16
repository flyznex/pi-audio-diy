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
  typedefs). **Update (post-implementation, verified against the real
  v1.2.6 header — see §7):** the real constructor is
  `AudioSourceSDFAT(const char* startFilePath = "/", const char* ext = ".mp3", int chipSelect = PIN_CS, int speedMHz = 10, int spi_mode = DEDICATED_SPI, bool setupIndex = true)`
  — two more parameters than originally guessed here (`spi_mode`,
  `setupIndex`), and the `speedMHz` default is `10`, not `2`. The
  `spi_mode` default (`DEDICATED_SPI`) turned out to matter a lot — see
  §7. It does its own directory traversal/filtering internally
  (`nextStream`/`previousStream`/`selectStream`, and it recurses into
  subdirectories, not just the given path), independent of this
  project's `listDir()`.
- `AudioPlayer::setPath(const char* path)` exists and selects a file by
  explicit path, separately from the source's own next/previous
  traversal.
- `MultiDecoder` lets one `AudioPlayer` handle multiple formats (MP3, WAV,
  ...) by mime-type-based dispatch to registered sub-decoders — needed
  eventually for WAV+MP3, but out of scope for this sub-phase (2a).

## 2. Goal (this sub-phase: 2a)

Get **WAV playback working end-to-end** on real hardware: select a `.wav`
file from the SD card's root directory and hear it play over I2S, with
`next`/`previous`/volume control exercised via Serial commands (no
physical buttons yet — that's Phase 3).

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

## 7. Known unknowns — RESOLVED (verified against the real v1.2.6 header)

**Update:** the final whole-branch review for this sub-phase cloned the
real upstream `arduino-audio-tools` at tag `v1.2.6` and checked every
assumption below against the actual header source, rather than
documentation. Recorded here as history — what this section originally
worried about, and what turned out to be true:

- **Include paths — this section didn't flag them as the risk, and they
  were the actual break.** The original code (guessing from an old
  community example) used `#include "AudioTools/AudioPlayer.h"` and
  `#include "AudioLibs/AudioSourceSDFAT.h"`. Neither exists in v1.2.6: the
  real paths are `AudioTools/CoreAudio/AudioPlayer.h` (already pulled in
  transitively by the umbrella `AudioTools.h`, so the include isn't even
  needed) and `AudioTools/Disk/AudioSourceSDFAT.h`. This was a hard
  compile-stop, fixed in the post-review fix wave. **Lesson for Phase
  2b:** check `MultiDecoder`'s real include path
  (`AudioTools/AudioCodecs/`, not `AudioLibs/`) before writing any code
  that assumes a path from an old example.
- **The constructor signature question resolved cleanly** — real
  signature confirmed as `AudioSourceSDFAT(const char *startFilePath="/", const char *ext=".mp3", int chipSelect=PIN_CS, int speedMHz=10, int spi_mode=DEDICATED_SPI, bool setupIndex=true)`.
  The call this project makes,
  `AudioSourceSDFAT<sd_t, file_t> source("/", ".wav", SD_CS_PIN, 4 /* speedMHz */, SHARED_SPI)`,
  matches this exactly.
- **The dual-SdFat-instance question was real, and worse than
  "wasteful."** `AudioSourceSDFAT` owns its own `AudioFs sd;` member and
  calls `sd.begin()` with `DEDICATED_SPI` by default — the same mode this
  project's own `file_sdfat.cpp` used for its independent `sd`/`root`
  instance on the same physical CS pin. `DEDICATED_SPI` assumes exclusive
  bus ownership; two objects both assuming that on the same bus is a real
  conflict, not a performance concern. Fixed: both instances now use
  `SHARED_SPI` (§3, §4's constructor call; `file_sdfat.h`'s `SD_CONFIG`
  macro).
- **An unanticipated collision the fix wave also had to close:** this
  project's own `#define SPI_CLOCK SD_SCK_MHZ(4)` (in `file_sdfat.h`)
  collided with a same-named macro the library itself unconditionally
  defines to `SD_SCK_MHZ(50)`. Since the library's header is included
  after `file_sdfat.h` in `fplayer.h`, the library's value silently won —
  the SD card would have been clocked at 50MHz instead of 4MHz with only
  a compiler warning as a hint. Fixed by renaming the project's macro to
  `SD_SPI_CLOCK`.
- **Root cause of the include-path break:** `platformio.ini`'s
  `arduino-audio-tools` dependency was an unpinned git URL — the original
  design was researched against a directory layout upstream has since
  reorganized. Now pinned to `#v1.2.6`, the exact version this section
  was verified against.

This section's original framing named the constructor signature as the
main risk to verify and treated include paths as an afterthought
("confirm ... the exact `#include` path this version expects" was one
clause in a Task 3 step). In hindsight the emphasis was backwards — worth
remembering for Phase 2b: verify include paths and version-specific
directory layout *first*, signatures second.

## 8. Error handling

`AudioPlayer`'s own methods (`begin`, `next`, `previous`, `setVolume`) all
return `bool`. The Serial harness in `main.cpp`'s `loop()` calls them
directly and ignores the return value, matching the library's own example
sketches — this is deliberately unchanged from the original design.

**Update (post-review):** `beginAudioPlayer()` itself does check the two
one-time setup calls that matter most for a first hardware bring-up —
`i2s.begin(cfg)` and `player.begin()` — and prints to `Serial1` on
failure, plus turns on the library's own `AudioLogger` (`Serial1`, `Info`
level) so the library's own `LOGE` calls (e.g. "sd.begin failed", "pins
bck/ws must be next to each other") surface too. This was added because a
silent "nothing plays" during Task 3's real hardware test is otherwise
indistinguishable between "no .wav files found," "SD/SPI conflict," and
"I2S misconfigured" — exactly the three things this sub-phase most needs
to tell apart on the first real run. Runtime navigation (`next`/`previous`/
`setVolume` in `loop()`) still ignores return values; that's out of scope
here.

## 9. Testing

No automated test suite exists in this project (documented in Phase 1's
plan; still true). Verification for 2a:

- `pio run -e release` and `pio run -e wokwi` compile cleanly — this is
  the first real compile of anything in `lib/Audio/`, since Phase 1's
  final review confirmed `lib/Audio` had never actually been reachable
  from the build graph before `main.cpp` includes `fplayer.h`.
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
