# Pico 2 W (RP2350) USB DAC (Hi-Res 96kHz / 24-bit Asynchronous UAC2) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Triển khai firmware độc lập biến Raspberry Pi Pico 2 W (chip RP2350) thành một Sound Card USB Audio Class 2.0 (UAC2) Hi-Res 96kHz / 24-bit Stereo với cơ chế Asynchronous Feedback Endpoint và xuất âm thanh ra I2S DAC.

**Architecture:** Bổ sung môi trường PlatformIO `[env:pico2w_usb_dac]` nhắm tới board `rpipico2w` với core Earle Philhower. Xây dựng file độc lập `src/usb_dac_main.cpp` tích hợp `Adafruit_TinyUSB` và `USBAudioStream` từ `arduino-audio-tools` v1.2.6, cấu hình UAC2 Asynchronous Mode, kết nối với `I2SStream` qua `StreamCopy` để đạt độ trễ thấp và chất lượng âm thanh cao nhất.

**Tech Stack:** C++, PlatformIO, Earle Philhower Arduino-Pico core, RP2350 (Pico 2 W), Adafruit TinyUSB, arduino-audio-tools v1.2.6.

**Spec:** `docs/superpowers/specs/2026-11-10-pico2w-usb-dac-design.md`

## Global Constraints

- **Board:** `rpipico2w` (Raspberry Pi Pico 2 W - RP2350)
- **Framework & Core:** `arduino`, `board_build.core = earlephilhower`
- **Audio Format:** 96.000 Hz, 24-bit PCM, 2 Channels (Stereo)
- **USB Mode:** UAC2 Asynchronous Mode với Explicit Feedback Endpoint (`ep_fb`), `enable_ep_in = false`, `enable_ep_out = true`
- **I2S Pinout:** `BCLK = GP2`, `LRCLK/WS = GP3`, `DOUT = GP8`, `MCLK = GP9`
- **Diagnostics UART:** `Serial1` trên `GP0` (TX) và `GP1` (RX) @ 115200 baud
- **Code Isolation:** `src_filter = +<usb_dac_main.cpp>` trong `[env:pico2w_usb_dac]`, không chạm vào logic SD Card hay OLED hiện có.

---

### Task 1: Cấu hình môi trường PlatformIO cho Pico 2 W (`platformio.ini`)

**Files:**
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: Cấu hình gốc trong `platformio.ini`
- Produces: Môi trường `[env:pico2w_usb_dac]` hỗ trợ biên dịch cho board `rpipico2w` với cờ `USE_TINYUSB=1` và cô lập mã nguồn bằng `src_filter`.

- [ ] **Step 1: Thêm cấu hình `[env:pico2w_usb_dac]` vào `platformio.ini`**

Thêm đoạn cấu hình sau vào cuối file `platformio.ini`:

```ini
[env:pico2w_usb_dac]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipico2w
framework = arduino
board_build.core = earlephilhower
platform_packages = framework-arduinopico@https://github.com/earlephilhower/arduino-pico.git

build_flags =
    -D USE_TINYUSB=1
    -D AUDIOINFO_DEFAULT_SAMPLERATE=96000
    -D AUDIOINFO_DEFAULT_CHANNELS=2
    -D AUDIOINFO_DEFAULT_BITS=24
    -D PIO_FRAMEWORK_ARDUINO_ENABLE_EXCEPTIONS

lib_deps =
    https://github.com/pschatzmann/arduino-audio-tools.git#v1.2.6
    adafruit/Adafruit TinyUSB Library

src_filter = +<usb_dac_main.cpp>
```

- [ ] **Step 2: Kiểm tra cú pháp cấu hình PlatformIO**

Chạy lệnh kiểm tra cấu hình môi trường:
```bash
pio project config
```
Expected: Cấu hình hiển thị danh sách các môi trường bao gồm `env:pico2w_usb_dac`.

- [ ] **Step 3: Commit thay đổi cấu hình**

```bash
git add platformio.ini
git commit -m "build(pio): add pico2w_usb_dac environment for RP2350 USB DAC

via [HAPI](https://hapi.run)

Co-Authored-By: HAPI <noreply@hapi.run>
Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 2: Tạo firmware kiểm thử độc lập USB DAC (`src/usb_dac_main.cpp`)

**Files:**
- Create: `src/usb_dac_main.cpp`

**Interfaces:**
- Consumes: `AudioTools.h`, `USBAudioStream.h`, `I2SStream.h`, `StreamCopy.h`
- Produces: Ứng dụng firmware hoàn chỉnh khởi tạo TinyUSB Audio UAC2 96kHz/24-bit và stream ra I2S DAC.

- [ ] **Step 1: Viết mã nguồn `src/usb_dac_main.cpp`**

Tạo file `src/usb_dac_main.cpp` với nội dung hoàn chỉnh:

```cpp
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "AudioTools.h"
#include "AudioTools/Communication/USB/USBAudioStream.h"

using namespace audio_tools;

// Định nghĩa thông số âm thanh Hi-Res
static const uint32_t SAMPLE_RATE = 96000;
static const uint8_t CHANNELS = 2;
static const uint8_t BITS_PER_SAMPLE = 24;

// Định nghĩa chân I2S theo thiết kế phần cứng
static const int PIN_I2S_BCLK = 2; // GP2
static const int PIN_I2S_WCLK = 3; // GP3 (WS/LRCLK)
static const int PIN_I2S_DOUT = 8; // GP8
static const int PIN_I2S_MCLK = 9; // GP9
static const int MCLK_MULT = 256;  // MCLK = 256 * 96000 = 24.576 MHz

// Đối tượng USB DAC và I2S
static USBAudioStream usb;
static I2SStream i2s;
static StreamCopy copier(i2s, usb);

// Biến thống kê trạng thái
static uint32_t last_stat_ms = 0;
static uint32_t bytes_copied_total = 0;

void setup() {
    // Khởi tạo UART chẩn đoán Serial1 (GP0/GP1 @ 115200)
    Serial1.begin(115200);
    delay(1000);
    Serial1.println("=========================================");
    Serial1.println("Pico 2 W (RP2350) Hi-Res USB DAC Initializing...");
    Serial1.printf("Target Audio: %u Hz, %u-bit, %u Channels\n", SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS);

    AudioLogger::instance().begin(Serial1, AudioLogger::Info);

    // 1. Cấu hình I2S DAC Output
    auto i2s_cfg = i2s.defaultConfig(TX_MODE);
    i2s_cfg.sample_rate = SAMPLE_RATE;
    i2s_cfg.channels = CHANNELS;
    i2s_cfg.bits_per_sample = BITS_PER_SAMPLE;
    i2s_cfg.pin_bck = PIN_I2S_BCLK;
    i2s_cfg.pin_ws = PIN_I2S_WCLK;
    i2s_cfg.pin_data = PIN_I2S_DOUT;
    i2s_cfg.pin_mck = PIN_I2S_MCLK;
    i2s_cfg.mck_multiplier = MCLK_MULT;
    i2s_cfg.buffer_size = 1024;
    i2s_cfg.buffer_count = 8;

    if (!i2s.begin(i2s_cfg)) {
        Serial1.println("[ERROR] i2s.begin() failed!");
    } else {
        Serial1.println("[OK] I2S DAC initialized successfully.");
    }

    // 2. Cấu hình USB Audio Class 2.0 (UAC2) Asynchronous
    USBAudioConfig usb_cfg;
    usb_cfg.sample_rate = SAMPLE_RATE;
    usb_cfg.channels = CHANNELS;
    usb_cfg.bits_per_sample = BITS_PER_SAMPLE;
    usb_cfg.enable_ep_out = true;       // Host -> Device (Phát nhạc)
    usb_cfg.enable_ep_in = false;      // Tắt Micro
    usb_cfg.enable_feedback_ep = true; // Bật Asynchronous Explicit Feedback
    usb_cfg.fifo_packets = 48;         // Bộ đệm ~48ms chống trễ
    usb_cfg.rx_start_fill_percent = 20;// Nạp đệm 20% trước khi xả ra I2S

    if (!usb.begin(usb_cfg)) {
        Serial1.println("[ERROR] usb.begin() failed!");
    } else {
        Serial1.println("[OK] USB Audio UAC2 initialized successfully.");
    }

    copier.resize(1024);
    last_stat_ms = millis();
    Serial1.println("[READY] USB DAC is running. Connect to PC via USB!");
    Serial1.println("=========================================");
}

void loop() {
    // Luân chuyển dữ liệu từ USB sang I2S
    size_t copied = copier.copy();
    if (copied > 0) {
        bytes_copied_total += copied;
    }

    // Thống kê nhịp tim định kỳ mỗi 5 giây ra Serial1
    uint32_t now = millis();
    if (now - last_stat_ms >= 5000) {
        last_stat_ms = now;
        Serial1.printf("[STATUS] Stream throughput: %u bytes/5s, Total: %u KB\n",
                       bytes_copied_total, (bytes_copied_total / 1024));
        bytes_copied_total = 0;
    }
}
```

- [ ] **Step 2: Biên dịch kiểm tra mã nguồn**

Chạy lệnh build PlatformIO cho môi trường `pico2w_usb_dac`:
```bash
pio run -e pico2w_usb_dac
```
Expected: Quá trình biên dịch hoàn tất thành công (`[SUCCESS]`) và sinh ra file nhị phân `.pio/build/pico2w_usb_dac/firmware.uf2`.

- [ ] **Step 3: Commit mã nguồn USB DAC**

```bash
git add src/usb_dac_main.cpp
git commit -m "feat(usb-dac): implement standalone Hi-Res 96kHz 24-bit USB DAC firmware for Pico 2 W

via [HAPI](https://hapi.run)

Co-Authored-By: HAPI <noreply@hapi.run>
Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 3: Thêm cơ chế xử lý chống nổ loa (Anti-Pop) & Callback trạng thái USB

**Files:**
- Modify: `src/usb_dac_main.cpp`

**Interfaces:**
- Consumes: TinyUSB device callbacks (`tud_mount_cb`, `tud_umount_cb`, `tud_suspend_cb`, `tud_resume_cb`)
- Produces: Cơ chế phát hiện cắm/rút cáp, xóa sạch buffer khi máy tính dừng phát để tránh phát ra tiếng lách tách ở loa.

- [ ] **Step 1: Thêm TinyUSB callbacks vào `src/usb_dac_main.cpp`**

Cập nhật `src/usb_dac_main.cpp` để bổ sung các callback giám sát sự kiện USB:

```cpp
// Callback khi máy tính kết nối và nhận diện thành công
void tud_mount_cb(void) {
    Serial1.println("[USB EVENT] Device mounted to Host PC.");
}

// Callback khi rút cáp hoặc ngắt kết nối USB
void tud_umount_cb(void) {
    Serial1.println("[USB EVENT] Device unmounted.");
    i2s.flush();
}

// Callback khi máy tính vào chế độ Sleep / Suspend
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    Serial1.println("[USB EVENT] USB suspended (Host sleep/idle).");
    i2s.flush();
}

// Callback khi máy tính thức dậy (Resume)
void tud_resume_cb(void) {
    Serial1.println("[USB EVENT] USB resumed.");
}
```

- [ ] **Step 2: Biên dịch kiểm tra lại firmware**

Chạy lệnh build để đảm bảo các callback không bị xung đột với `Adafruit_TinyUSB`:
```bash
pio run -e pico2w_usb_dac
```
Expected: `[SUCCESS]`, không có lỗi liên kết (link errors) hay redefined symbols.

- [ ] **Step 3: Commit cập nhật chống nổ loa**

```bash
git add src/usb_dac_main.cpp
git commit -m "feat(usb-dac): add USB connection lifecycle callbacks and anti-pop buffer flushing

via [HAPI](https://hapi.run)

Co-Authored-By: HAPI <noreply@hapi.run>
Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 4: Hướng dẫn nạp và nghiệm thu phần cứng (Hardware Verification Guide)

**Files:**
- Create: `docs/PICO2W_USB_DAC_TESTING.md`

**Interfaces:**
- Consumes: File nhị phân `.pio/build/pico2w_usb_dac/firmware.uf2`
- Produces: Tài liệu từng bước hướng dẫn người dùng kết nối mạch, nạp firmware và nghiệm thu âm thanh Hi-Res trên máy tính.

- [ ] **Step 1: Viết tài liệu hướng dẫn nghiệm thu `docs/PICO2W_USB_DAC_TESTING.md`**

Tạo file `docs/PICO2W_USB_DAC_TESTING.md` với các hướng dẫn chi tiết:
1. Sơ đồ nối dây giữa Pico 2 W và I2S DAC (GP2, GP3, GP8, GP9, GND, 3V3/5V).
2. Hướng dẫn đưa Pico 2 W vào chế độ BOOTSEL và nạp file `.uf2`.
3. Cách kiểm tra thiết bị nhận diện trên Windows, macOS và Linux (96000 Hz, 24-bit).
4. Các bài nhạc test đề xuất và tiêu chí kiểm tra độ sạch của âm thanh.

- [ ] **Step 2: Commit tài liệu hướng dẫn**

```bash
git add docs/PICO2W_USB_DAC_TESTING.md
git commit -m "docs: add testing and flashing guide for Pico 2 W USB DAC

via [HAPI](https://hapi.run)

Co-Authored-By: HAPI <noreply@hapi.run>
Co-Authored-By: Claude <noreply@anthropic.com>"
```
