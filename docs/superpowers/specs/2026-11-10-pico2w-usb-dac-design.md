# Design Spec: Pico 2 W (RP2350) USB DAC (Hi-Res 96kHz / 24-bit Asynchronous UAC2)

- **Date:** 2026-11-10
- **Status:** Approved
- **Target Hardware:** Raspberry Pi Pico 2 W (RP2350, 520 KB SRAM)
- **Audio Output:** I2S DAC Module (PCM5102A, MAX98357A, CS4344, etc.)
- **Scope:** Milestone 1 — Standalone USB DAC Test Firmware

---

## 1. Mục tiêu & Bối cảnh

### 1.1. Bối cảnh
Dự án `pi-audio-diy` ban đầu là một trình phát nhạc WAV từ thẻ SD qua giao tiếp I2S trên chip RP2040. Người dùng mong muốn nâng cấp hệ thống để tự động chuyển đổi giữa chế độ **USB DAC** (khi cắm vào máy tính) và **Wi-Fi Streaming / Web Radio** (khi không có tín hiệu USB), đồng thời đã nâng cấp phần cứng lên **Raspberry Pi Pico 2 W (chip RP2350)** và ưu tiên chất lượng âm thanh Hi-Res cao nhất có thể.

### 1.2. Mục tiêu của Milestone 1
Trước khi tích hợp Wi-Fi và State Machine đa nguồn, Milestone 1 tập trung xây dựng và nghiệm thu một bản firmware kiểm thử độc lập:
* Biến Pico 2 W thành một **Sound Card USB Audio Class 2.0 (UAC2)** chuẩn cắm-là-chạy (driverless) trên Windows, macOS, Linux, Android và iOS.
* Đạt định dạng âm thanh chuẩn phòng thu / Hi-Res cao nhất trên nền tảng USB 1.1 Full-Speed: **96.000 Hz, 24-bit Stereo**.
* Hoạt động ở chế độ **Asynchronous Mode với Explicit Feedback Endpoint** để đồng bộ xung nhịp phần cứng, triệt tiêu hoàn toàn jitter và hiện tượng nổ hạt/lệch xung (clock drift).
* Xuất tín hiệu PCM qua I2S phần cứng giữ nguyên sơ đồ chân hiện tại.
* Không làm ảnh hưởng tới mã nguồn máy phát nhạc thẻ nhớ SD hiện có trong repo.

---

## 2. Giới hạn phần cứng & Lựa chọn kỹ thuật

### 2.1. Giới hạn bus USB 1.1 Full-Speed trên RP2350
* Bộ điều khiển USB tích hợp của RP2350 là USB 1.1 Full-Speed (12 Mbps), chu kỳ frame là 1ms.
* Dung lượng tối đa của một gói tin Isochronous Endpoint là 1023 bytes/ms.
* Băng thông gói tin theo tần số:
  * **96 kHz / 24-bit Stereo**: $96 \times 3 \times 2 = 576$ bytes/ms (+ headroom đồng bộ $\approx 582$ bytes/ms) $\rightarrow$ An toàn trong ngưỡng 1023 bytes/ms.
  * **192 kHz / 24-bit Stereo**: $192 \times 3 \times 2 = 1152$ bytes/ms $\rightarrow$ Vượt quá ngưỡng 1023 bytes/ms của USB 1.1.
* **Kết luận:** Mức **96 kHz / 24-bit Stereo** là trần Hi-Res tối đa về mặt vật lý của RP2350.

### 2.2. Cơ chế khóa xung Asynchronous Feedback
* Trong chế độ Synchronous hoặc Adaptive, tốc độ lấy mẫu bị chi phối bởi clock máy tính (vốn chứa nhiều nhiễu jitter).
* Thiết kế sử dụng chuẩn **UAC2 Asynchronous Mode**:
  * Pico 2 W sử dụng xung nhịp phần cứng I2S cố định (sinh ra từ PIO / Clock divider của RP2350).
  * Mở thêm một kênh phản hồi chuyên dụng (`ep_fb`). Mỗi frame, Pico 2 W gửi thông điệp feedback về Host báo cáo chính xác tốc độ tiêu thụ buffer thực tế.
  * Máy tính sẽ tự điều chỉnh số mẫu gửi xuống trong frame tiếp theo khớp 100% với tốc độ DAC.

---

## 3. Kiến trúc hệ thống & Môi trường xây dựng

### 3.1. Cấu hình PlatformIO (`platformio.ini`)
Thêm môi trường `[env:pico2w_usb_dac]` độc lập:

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

### 3.2. Sơ đồ chân phần cứng (Pinout)
Giữ nguyên sơ đồ chân I2S đã định nghĩa trong dự án:

| Tín hiệu | Chân GPIO Pico 2 W | Chức năng I2S DAC | Ghi chú |
| :--- | :--- | :--- | :--- |
| **BCLK** | `GP2` | Bit Clock (SCK) | Clock nhịp bit dữ liệu |
| **WCLK / LRCLK** | `GP3` | Word Select (WS/LRCK) | Xác định kênh Trái / Phải |
| **DOUT** | `GP8` | Data Out (DIN) | Dữ liệu âm thanh số PCM |
| **MCLK** | `GP9` | Master Clock | Cung cấp xung tham chiếu cho DAC cao cấp |
| **UART TX** | `GP0` | Serial1 Debug TX | In log chẩn đoán trạng thái (115200 baud) |
| **UART RX** | `GP1` | Serial1 Debug RX | Nhận lệnh điều khiển kiểm thử |

---

## 4. Thiết kế phần mềm chi tiết (`src/usb_dac_main.cpp`)

### 4.1. Khởi tạo & Cấu hình UAC2 (`USBAudioStream`)
* Sử dụng lớp `USBAudioStream` của `arduino-audio-tools` (dựa trên `USBAudioDeviceTinyUSB`).
* Cấu hình tham số:
  ```cpp
  USBAudioConfig cfg;
  cfg.sample_rate = 96000;
  cfg.channels = 2;
  cfg.bits_per_sample = 24;
  cfg.enable_ep_out = true;       // Host -> Device (Phát âm thanh)
  cfg.enable_ep_in = false;      // Tắt Micro để tối ưu tài nguyên
  cfg.enable_feedback_ep = true; // Kích hoạt Explicit Asynchronous Feedback
  cfg.fifo_packets = 48;         // Buffer 48ms (~28 KB trên RAM 520 KB)
  cfg.rx_start_fill_percent = 20;// Nạp đệm 20% trước khi xả ra I2S
  ```

### 4.2. Khởi tạo I2S Stream (`I2SStream`)
* Cấu hình `I2SStream` chế độ `TX_MODE`:
  ```cpp
  auto i2s_cfg = i2s.defaultConfig(TX_MODE);
  i2s_cfg.sample_rate = 96000;
  i2s_cfg.channels = 2;
  i2s_cfg.bits_per_sample = 24;
  i2s_cfg.pin_bck = 2;
  i2s_cfg.pin_ws = 3;
  i2s_cfg.pin_data = 8;
  i2s_cfg.pin_mck = 9;
  ```

### 4.3. Pipeline streaming và vòng lặp `loop()`
* Sử dụng `StreamCopy copier(i2s, usb);` để chuyển dữ liệu từ USB RingBuffer sang I2S DMA.
* Vòng lặp `loop()` tập trung tối đa CPU cho việc duy trì throughput dòng dữ liệu:
  ```cpp
  void loop() {
      copier.copy();
  }
  ```

### 4.4. Cơ chế chống nổ loa (Anti-Pop / Click) & Xử lý sự kiện cắm/rút
1. **Host Play / Pause (Alternate Setting Change)**:
   * Khi người dùng bấm Pause: Host hạ alternate setting về 0 $\rightarrow$ Firmware ngưng nạp dữ liệu vào DMA, ghi đệm 0 (Silence) để tránh xé tiếng.
   * Khi người dùng bấm Play: Host chuyển sang alternate setting 1 $\rightarrow$ Firmware đợi nạp đầy ngưỡng `rx_start_fill_percent = 20%` rồi mới bắt đầu truyền ra I2S.
2. **Buffer Underflow Protection**:
   * Nếu máy tính bị nghẽn CPU tạm thời làm rỗng buffer, bộ đệm tự động chèn mẫu câm (silence frame) thay vì để dữ liệu rác truyền ra DAC.
3. **Cắm / Rút cáp (Mount / Suspend / Unmount)**:
   * Khi rút cáp: I2S DMA dừng ngay lập tức trong êm ái.

---

## 5. Kế hoạch kiểm thử (Test Plan)

### 5.1. Biên dịch và nạp
1. Biên dịch: `pio run -e pico2w_usb_dac`.
2. Nạp file `.uf2` vào Pico 2 W qua chế độ BOOTSEL.

### 5.2. Các bước xác minh thực tế
1. **Kiểm tra nhận diện hệ điều hành**:
   * Cắm cáp USB vào máy tính Windows / macOS / Linux.
   * Kiểm tra thiết bị âm thanh nhận diện đúng tên và hiển thị định dạng hỗ trợ: **24-bit, 96000 Hz**.
2. **Kiểm tra chất lượng âm thanh**:
   * Phát nhạc chất lượng cao từ YouTube / Spotify / foobar2000 (FLAC Hi-Res).
   * Lắng nghe qua cổng ra I2S: Tín hiệu sạch, âm thanh tách bạch, không có tiếng xì nền và không có tiếng nổ lách tách do lệch xung.
3. **Kiểm tra thao tác biên**:
   * Bấm Play / Pause / Tua nhạc liên tục: Không phát ra tiếng "bốp/pop" giật màng loa.
   * Rút cáp USB khi đang phát nhạc và cắm lại: Hệ thống tự phục hồi bình thường.
4. **Kiểm tra Serial Log**:
   * Mở cổng `Serial1` (GP0/GP1 @ 115200 baud) để xác nhận số lượng frame xử lý và kiểm tra không có lỗi underflow.

---

## 6. Tiêu chí nghiệm thu (Acceptance Criteria)

- [ ] Dự án biên dịch thành công môi trường `pico2w_usb_dac` không có cảnh báo lỗi.
- [ ] Máy tính nhận diện Pico 2 W là Sound Card UAC2 96kHz / 24-bit Stereo.
- [ ] Âm thanh phát từ máy tính truyền mượt mà tới I2S DAC, dải âm đầy đặn, không méo tiếng.
- [ ] Chuyển trạng thái Play/Pause/Cắm/Rút cáp an toàn, êm dịu, không gây sốc xung ra loa.
