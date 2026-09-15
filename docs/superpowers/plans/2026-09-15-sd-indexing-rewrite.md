# SD Indexing Rewrite (Phase 1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the broken, unused persistent `.index`-file SD listing scheme with a simple, correct on-demand directory listing (`listDir()`), and wire it into `main.cpp` so the player can actually enumerate SD card contents without crashing.

**Architecture:** `lib/File/file_sdfat.{h,cpp}` drops all offset/text-index machinery and exposes one function, `listDir()`, built directly on SdFat's `openNext()` iteration. `src/main.cpp` calls it to print and display the root directory's contents. `lib/Audio/fplayer.cpp` is touched only to stop referencing the functions being removed (real track-selection logic is Phase 2, out of scope here).

**Tech Stack:** PlatformIO (`pio` CLI), Arduino framework on RP2040 (`earlephilhower` core), SdFat library. No unit-test framework is set up in this project (`test/` is the stock PlatformIO placeholder) and the code is hardware/SPI-dependent, so this plan's "test cycle" is: compile both PlatformIO environments cleanly after each task, then confirm behavior via the Wokwi simulator (`wokwi.toml` + `diagram.json` already define an `env:wokwi` target with a `wokwi-microsd-card` part). If `pio` isn't installed in your execution shell, run these build steps in the environment where PlatformIO is set up (e.g. the VS Code PlatformIO extension terminal) before marking a step done.

**Spec:** `docs/superpowers/specs/2026-09-15-sd-audio-player-continuation-design.md` (§4, "Phase 1 — SD indexing")

## Global Constraints

- Both `pio run -e release` and `pio run -e wokwi` must build with zero errors after every task in this plan.
- No persistent `.index` file — directory listing is always live via SdFat `openNext()` (spec §4.2, Approach B).
- `FILE_NAME_DIM = 50` stays the max filename buffer size (`lib/File/file_sdfat.h`).
- MP3/WAV decoding, playback, and physical-button input are explicitly out of scope for this plan (spec §5, §6) — only stub out what's needed to keep the tree compiling.

---

### Task 1: Rewrite the `file_sdfat` listing API and fix the resulting broken call site in `FPlayer`

**Files:**
- Modify: `lib/File/file_sdfat.h`
- Modify: `lib/File/file_sdfat.cpp`
- Modify: `lib/Audio/fplayer.cpp:13-19`

**Interfaces:**
- Consumes: nothing from other tasks in this plan.
- Produces (used by Task 2):
  - `struct DirEntry { char name[FILE_NAME_DIM]; bool isDir; };`
  - `int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex = 0);`
  - `bool initCard();` (unchanged signature, kept)
  - `extern sd_t sd;` / `extern file_t root;` (unchanged, kept)

- [ ] **Step 1: Replace `lib/File/file_sdfat.h` with the new API**

Replace the full file contents with:

```cpp
#include <SPI.h>

#include "SdFat.h"

#define SPI_CLOCK SD_SCK_MHZ(4)

const size_t FILE_NAME_DIM = 50;
const uint8_t SD_CS_PIN = 5;
const uint8_t SD_MOSI_PIN = 7;
const uint8_t SD_MISO_PIN = 4;
const uint8_t SD_SCK_PIN = 6;

#if SD_FAT_TYPE == 0
typedef SdFat sd_t;
typedef File file_t;
#elif SD_FAT_TYPE == 1
typedef SdFat32 sd_t;
typedef File32 file_t;
#elif SD_FAT_TYPE == 2
typedef SdExFat sd_t;
typedef ExFile file_t;
#elif SD_FAT_TYPE == 3
typedef SdFs sd_t;
typedef FsFile file_t;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

extern sd_t sd;
extern file_t root;

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

bool initCard();

struct DirEntry {
    char name[FILE_NAME_DIM];
    bool isDir;
};

/// @brief List up to maxEntries children of `path`, skipping the first startIndex.
/// @param path directory to list
/// @param out caller-allocated array of at least maxEntries DirEntry
/// @param maxEntries capacity of `out`
/// @param startIndex how many entries to skip before filling `out`
/// @return number of entries written into `out`
int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex = 0);
```

- [ ] **Step 2: Replace `lib/File/file_sdfat.cpp` with the new implementation**

Replace the full file contents with:

```cpp
#include "file_sdfat.h"
#include "SdFat.h"

sd_t sd;
file_t root;

bool initCard()
{
    SPI.setCS(SD_CS_PIN);
    SPI.setTX(SD_MOSI_PIN);
    SPI.setRX(SD_MISO_PIN);
    SPI.setSCK(SD_SCK_PIN);
    SPI.begin();
    if (!sd.begin(SD_CONFIG))
        return false;
    return root.open("/");
}

int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex)
{
    file_t dir;
    if (!dir.open(path, O_RDONLY))
    {
        Serial1.println("Failed to open directory");
        return 0;
    }
    dir.rewind();

    file_t entry;
    int skipped = 0;
    int filled = 0;
    while (filled < maxEntries && entry.openNext(&dir, O_RDONLY))
    {
        if (skipped < startIndex)
        {
            skipped++;
            entry.close();
            continue;
        }
        entry.getName(out[filled].name, FILE_NAME_DIM);
        out[filled].isDir = entry.isDir();
        entry.close();
        filled++;
    }
    dir.close();
    return filled;
}
```

- [ ] **Step 3: Fix the now-broken call site in `lib/Audio/fplayer.cpp`**

`FPlayer::begin()` (lines 13-19) calls `getFileName()` and `readLineByIndex()`, which no longer exist after Step 1-2. Replace the method body:

```cpp
bool FPlayer::begin(int index, bool isActive)
{
    // Track selection lands in Phase 2 — see
    // docs/superpowers/specs/2026-09-15-sd-audio-player-continuation-design.md §5.
    return false;
}
```

- [ ] **Step 4: Build both environments**

Run: `pio run -e release`
Expected: `SUCCESS`, zero errors.

Run: `pio run -e wokwi`
Expected: `SUCCESS`, zero errors.

- [ ] **Step 5: Commit**

```bash
git add lib/File/file_sdfat.h lib/File/file_sdfat.cpp lib/Audio/fplayer.cpp
git commit -m "refactor(sd): replace index-file scheme with direct listDir()

Drops the buggy persistent .index text-file cache in favor of
listing directories on demand via SdFat openNext(). Fixes the
undefined file/idxFile externs, the comma-operator bug on open(),
the unallocated lineOffsets pointer, and the off-by-one paging loop.
Updates FPlayer::begin() to stop calling the removed API (real
implementation is Phase 2)."
```

---

### Task 2: Wire `listDir()` into `main.cpp` and fix the uninitialized-pointer crash

**Files:**
- Modify: `src/main.cpp:91-115` (the `setup()` function)

**Interfaces:**
- Consumes: `struct DirEntry`, `int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex = 0)` from Task 1 (`lib/File/file_sdfat.h`, already `#include`d in `main.cpp:4`).
- Produces: nothing consumed by later tasks in this plan.

- [ ] **Step 1: Replace the SD-listing block in `setup()`**

Current code (`src/main.cpp:108-114`):

```cpp
  root.open("/");
  root.ls(&Serial1,LS_R|LS_SIZE);
  tex.clear();
  char *n;
  root.getName(n,10);
  tex.setText(n);
  tex.draw();
```

Replace with:

```cpp
  const int MAX_ENTRIES = 16;
  DirEntry entries[MAX_ENTRIES];
  int count = listDir("/", entries, MAX_ENTRIES);
  Serial1.print("entries found: ");
  Serial1.println(count);
  for (int i = 0; i < count; i++) {
    Serial1.print(entries[i].isDir ? "[DIR] " : "[FILE] ");
    Serial1.println(entries[i].name);
  }
  tex.clear();
  if (count > 0) {
    tex.setText(entries[0].name);
    tex.draw();
  }
```

(`root.open("/")` is removed here since `initCard()` — called earlier in `setup()` — already opens `root` on `"/"`; keeping a second call was redundant. `char *n` — the uninitialized pointer whose `getName` call would have crashed — is gone entirely.)

- [ ] **Step 2: Build both environments**

Run: `pio run -e release`
Expected: `SUCCESS`, zero errors.

Run: `pio run -e wokwi`
Expected: `SUCCESS`, zero errors.

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "fix(main): list SD root via listDir(), remove crash-prone getName call

Replaces the uninitialized char* passed to root.getName() (guaranteed
crash) with the new listDir() API, printing all entries to Serial1
and rendering the first one on the OLED."
```

---

### Task 3: Verify directory listing end-to-end in the Wokwi simulator

**Files:** none (verification-only task)

**Interfaces:**
- Consumes: the built `wokwi` firmware from Task 2.
- Produces: nothing (terminal task for this plan).

- [ ] **Step 1: Launch the Wokwi simulation**

Using the VS Code Wokwi extension (or `wokwi-cli` if installed) with this project's `wokwi.toml` and `diagram.json`, start a simulation. The `wokwi-microsd-card` part in `diagram.json` needs an SD card image attached per the Wokwi microSD docs (a local folder mapped as the card's contents) — put a couple of test files and one subfolder in it if not already configured, so the listing has something real to show.

- [ ] **Step 2: Confirm serial output**

Expected in the serial monitor:
```
entries found: <N>
[FILE] <name>
[DIR] <name>
...
```
where `<N>` matches the number of files/folders you put on the simulated card, and every name printed matches what's on the card (no garbage bytes, no truncation, no crash/reset).

- [ ] **Step 3: Confirm OLED output**

The simulated SSD1306 should show the first listed entry's name at the top-left, replacing the earlier "init done" text (via `tex.clear()` + `tex.setText()` + `tex.draw()` in Task 2's code).

- [ ] **Step 4 (optional): Confirm on real hardware**

If you have the physical Pico + SD + OLED wired up, flash the `release` build and confirm the same two things (serial listing, OLED shows first entry) with a real SD card.

- [ ] **Step 5: Report back**

No commit for this task — it's a verification checkpoint. Note in the PR/commit history or to your collaborator whether Steps 2-3 (and optionally 4) matched expectations before starting Phase 2.
