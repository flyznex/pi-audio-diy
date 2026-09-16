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

**Composition, not inheritance.** The existing skeleton
(`class FPlayer : public AudioPlayer`) does not work with the real
constructor: a base class's constructor always runs before a derived
class's own members are initialized, so `FPlayer` cannot pass references
to its *own* `source`/`decoder`/`i2s` members into `AudioPlayer`'s
3-argument base constructor. Every real usage example wires these as
sibling objects (globals, in the library's own examples), not as a
base/derived pair.

`FPlayer` becomes a plain composition class:

```cpp
class FPlayer {
public:
    FPlayer();
    bool setWorkingDir(const char *path);
    bool begin(int index = 0);
    bool next(int offset = 1);
    bool previous(int offset = 1);
    bool setVolume(float volume);
    void copy();                 // ticks the underlying player; call every loop()
    const char *currentName() const;
private:
    static const int MAX_ENTRIES = 16;
    DirEntry entries[MAX_ENTRIES];
    int entryCount = 0;
    int currentIndex = -1;
    char workingDir[FILE_NAME_DIM];

    AudioSourceSDFAT<sd_t, file_t> source;
    WAVDecoder decoder;
    I2SStream i2s;
    AudioPlayer player;

    bool isPlayable(const DirEntry &e) const;  // !isDir && name ends with ".wav" (case-insensitive)
    bool selectIndex(int idx);                 // builds full path, player.setPath(...), player.play()
};
```

The old `I2SAudio` wrapper class (`lib/Audio/i2s_audio_init.{h,cpp}`) is
**deleted**. Its only job — deferring `i2s.begin(cfg)` until an explicit
call — has no value once `I2SStream` is used directly as `AudioPlayer`'s
output the way the library's own examples do. Its pin constants
(`PIN_MCLK`, `PIN_WCLK`, `PIN_DOUT`, `PIN_BCLK`, `MCLK_MULTI`) move into
`fplayer.h` since they're only meaningful there now.

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

## 5. Navigation model — bridging with `listDir()`

Rather than trusting `AudioSourceSDFAT`'s own traversal/filtering (whose
exact multi-format behavior is unconfirmed — see §7), `FPlayer` drives
selection itself from Phase 1's `listDir()`:

- `setWorkingDir(path)` calls `listDir(path, entries, MAX_ENTRIES)`,
  stores the count, resets `currentIndex` to `-1`.
- `begin(index)` bounds-checks `index` against `entryCount`; if
  `entries[index]` is playable (`isPlayable()`: not a directory, name
  ends in `.wav`, case-insensitive), it calls `selectIndex(index)` —
  which builds the full path (`workingDir + "/" + entries[index].name`),
  calls `player.setPath(fullPath)` then `player.play()`, and returns
  whatever those calls report. If not playable, `begin()` returns `false`
  without touching the player.
- `next(offset)`/`previous(offset)` walk `entries[]` from `currentIndex`
  in the requested direction, skipping non-playable entries (directories,
  non-`.wav` files), and land on `selectIndex()` for the first playable
  entry found. Reaching either end of `entries[]` without finding one
  stops there and returns `false` (no wraparound in 2a — simple and
  matches the "flat directory browsing only" scope already set in the
  parent spec's out-of-scope list).
- `copy()` is a thin wrapper over `player.copy()` — must be called every
  `loop()` iteration for playback to actually progress (this is how the
  library's pull-driven model works: nothing plays unless something
  calls `copy()` regularly).

This sidesteps most of the `AudioSourceSDFAT` ext-filter uncertainty:
`FPlayer` always resolves *which* file to play itself, and only asks the
underlying `player` to open one exact, already-known-good path — it never
relies on the source's own filtering or enumeration order.

## 6. Wiring in `main.cpp`

`setup()` gains, after `initCard()` succeeds:

```cpp
FPlayer player;
player.setWorkingDir("/");
player.setVolume(0.5f);   // conservative starting volume, not persisted
player.begin(0);
```

`loop()` becomes:

```cpp
void loop() {
  player.copy();
  if (Serial1.available()) {
    char c = Serial1.read();
    switch (c) {
      case 'n': player.next(); break;
      case 'p': player.previous(); break;
      case '+': player.setVolume(/* current + step, clamped to 1.0 */); break;
      case '-': player.setVolume(/* current - step, clamped to 0.0 */); break;
    }
    Serial1.println(player.currentName());
  }
}
```

(Exact volume step/clamp logic and whether `FPlayer` exposes a getter for
current volume is an implementation-plan-level detail, not fixed here.)

`FPlayer player;` needs to be a global/static object (not a local in
`setup()`), matching every real usage example of `AudioPlayer` in this
library — the object's lifetime must span `setup()` and `loop()`.

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
- Whether `setPath()` succeeds regardless of the `ext` filter the source
  was constructed with, or whether it validates the extension and
  rejects a path that doesn't match. If it validates, `AudioSourceSDFAT`
  must be constructed with an `ext` value permissive enough to accept
  `.wav` (exact mechanism — wildcard string, empty string, or something
  else — to be read from the real header, not guessed).
- The exact constructor call this project needs:
  `AudioSourceSDFAT<sd_t, file_t> source("/", <ext-value-per-above>, SD_CS_PIN, <speed>)`
  — confirm parameter order and types against the real header before
  transcribing into `fplayer.cpp`.

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
