# SD Card Audio Player (Pi Pico) — Continuation Design

Date: 2026-09-15
Status: Approved for Phase 1 implementation

## 1. Context

Firmware for a DIY SD-card music player built on a Raspberry Pi Pico
(RP2040, Arduino framework, `earlephilhower` core). Stack:

- Audio: `arduino-audio-tools` (pschatzmann) over I2S
- Storage: `SdFat` over SPI
- Display: SSD1306 OLED (I2C for Wokwi sim, SPI for real hardware) via
  a small custom `Element` widget hierarchy (`Icon`, `TextElem`,
  `IconText`, `IconTextSelectable`)
- Two PlatformIO environments: `release` (real hardware, SPI OLED) and
  `wokwi` (simulator, I2C OLED)

The repo already contains skeleton classes for every subsystem
(`FPlayer : AudioPlayer`, `I2SAudio`, `file_sdfat.*`, `element.*`) but
several are unfinished or buggy, and nothing is wired together yet —
`src/main.cpp` only initializes the display and SD card.

Hardware pin assignment lives in code as constants
(`lib/File/file_sdfat.h`, `lib/Audio/i2s_audio_init.h`,
`lib/Display/ssd1306.h`) — already confirmed working on real hardware
for SD+display, and per the developer, SD+I2S together as well.

## 2. Goal

Get the player to a working end-to-end state: browse folders/files on
the SD card from the OLED, select a track with physical buttons, and
play it back (WAV first, MP3 next) over I2S.

## 3. Scope decomposition

The remaining work splits into four largely independent phases, built
in this order because each depends on the previous one being solid:

1. **SD indexing** — list files/folders on the card reliably.
2. **Audio playback** — wire `FPlayer`/`I2SAudio` to actually decode
   and play a track picked by index (WAV, then MP3).
3. **Input/UI** — physical buttons (next/prev/select/vol+/vol-) driving
   the existing `Element` widgets to browse and control playback.
4. **Future/out of scope for now**: DSF format support, index caching
   for very large libraries, volume persistence, RTC-based cache
   invalidation.

Each phase is its own bounded implementation cycle with its own
design-in-chat/approval; this document formalizes the phase breakdown
and gives a detailed design for **Phase 1**, which is approved to
start now. Phases 2–3 are captured here as scope/approach so the
overall shape is on record, but their detailed design happens when
each phase starts.

## 4. Phase 1 — SD indexing (detailed design, approved)

### 4.1 Problem with the current code

`lib/File/file_sdfat.cpp` implements a persistent `.index` text-file
scheme (scan a directory once, write `prefix|name` lines to a cache
file, then read entries back by byte offset). It has several concrete
bugs:

- `file` / `idxFile` are declared `extern` in the header but never
  defined anywhere → link error.
- `idxFile.open(idxFileName.c_str()), O_RDWR | O_CREAT` uses the comma
  operator by mistake; the open flags are silently dropped.
- `indexFiles()` recurses into subdirectories passing the bare child
  name instead of a joined path, corrupting the recursive scan.
- `loadIndexed()` writes into `long *lineOffsets`, which is never
  allocated — undefined behavior.
- `readPagingWithIndex()` has an off-by-one loop bound and indexes the
  output array with the absolute row index instead of a
  page-relative one, and never actually uses the passed-in `offset`
  array to seek to the right line.
- `isFile()` always returns `true` regardless of the prefix.
- `src/main.cpp` calls `root.getName(n, 10)` with `char *n` that was
  never allocated → crash.

### 4.2 Chosen approach: drop the persistent index file

Instead of fixing the offset/text-parsing scheme in place, Phase 1
replaces it with direct, on-demand directory listing via SdFat's
`openNext()`, with no persistent `.index` file and no byte-offset
bookkeeping. Rationale: this is a personal player, not a library with
tens of thousands of files per folder, so the cost of re-scanning a
directory each time it's opened is negligible, and it removes most of
the bug classes outright rather than patching them one at a time.

Two more elaborate alternatives were considered and rejected for now:
keeping the text-index file but fixing all offset bugs (keeps more of
the original design, but keeps the bug-prone offset/text-parsing
machinery), and switching to a fixed-size binary record index file
(gets O(1) paginated access without offset bugs, but adds cache
invalidation complexity with no RTC on this board to know when to
refresh it). Both can be revisited later if directory sizes actually
become a performance problem.

### 4.3 New API

`lib/File/file_sdfat.h` replaces the old index-file API
(`INDEX_DB_FILE_NAME`, `readLine`, `readLineByIndex`,
`readPagingWithIndex`, `isFile`, `getFileName`, `addRecord`,
`addPrefix`, `loadIndexed`, and the `file`/`idxFile` externs) with:

```cpp
struct DirEntry {
    char name[FILE_NAME_DIM];
    bool isDir;
};

// Fills `out` with up to `maxEntries` entries of `path`, starting
// after skipping `startIndex` entries. Returns the number filled.
int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex = 0);
```

`initCard()` is unchanged (pin setup here is already correct).

### 4.4 Implementation notes

- `listDir()` opens `path`, calls `rewind()`, then loops
  `openNext(&entry, O_RDONLY)`: skip the first `startIndex` results,
  copy up to `maxEntries` names (`entry.getName(out[i].name,
  FILE_NAME_DIM)`, `out[i].isDir = entry.isDirectory()`) into the
  output array, `close()` each entry as it's consumed, then close the
  directory handle.
- `src/main.cpp` replaces the current `root.ls(...)` /
  `root.getName(n, 10)` block with a `listDir("/", entries, N)` call,
  printing results to `Serial1` and rendering the first entry via
  `TextElem` — enough to confirm the card is readable end to end.

### 4.5 Testing

- `pio run -e wokwi` and `pio run -e release` both build cleanly.
- Wokwi simulation (existing `wokwi-microsd-card` part in
  `diagram.json`) confirms `Serial1` prints the right file/folder
  names and the OLED shows the first entry.
- Optional: flash `release` to real hardware with a real SD card and
  compare output.

## 5. Phase 2 — Audio playback (scope, design deferred)

Goal: `FPlayer::begin(index)` actually plays the track at `index` in
the current working directory (from Phase 1's `listDir`) over I2S;
`next`/`previous` change track.

Known open questions to resolve when this phase starts:
- The real constructor/override contract of `AudioPlayer` in
  `arduino-audio-tools` — the current `FPlayer` subclass calls a
  parameterless `AudioPlayer()` constructor which needs to be checked
  against the actual library API (source, decoder, output wiring).
- `FPlayer::path` is an unallocated `uint8_t *` that
  `setWorkingDir()` currently `memcpy`s into — needs a real buffer.
- Decoder selection by file extension: `WAVDecoder` for `.wav`,
  `MP3DecoderHelix` (already available via `arduino-audio-tools`) for
  `.mp3`. DSF is explicitly out of scope for this phase.
- `I2SAudio::begin()` needs to actually be called from `setup()`, and
  the decode/copy loop driven from `loop()`.

## 6. Phase 3 — Input/UI (scope, design deferred)

Goal: physical buttons drive folder/track browsing and playback
control.

- Discrete buttons (not a rotary encoder): next, previous, select,
  volume up, volume down — digital GPIO inputs with debounce.
- `diagram.json` needs these added as parts/connections for Wokwi
  testing.
- Rendering: reuse the existing `IconTextSelectable` list widget to
  show the current directory's entries (from Phase 1's `listDir`),
  move selection with next/prev, `select` calls `FPlayer::begin(index)`
  (from Phase 2).

## 7. Out of scope (for now)

- DSF/DSD file support.
- Index caching for very large SD libraries (would revisit approach
  from §4.2 if this becomes a real problem).
- Volume/playback-state persistence across power cycles.
- Any UI beyond flat directory browsing (search, sorting, playlists).
