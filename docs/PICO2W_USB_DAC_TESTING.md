# Pico 2 W USB DAC: Hướng dẫn nạp và nghiệm thu phần cứng

**Phiên bản:** v1.0  
**Ngày cập nhật:** 2026-11-10  
**Mục tiêu:** Hướng dẫn chi tiết kết nối phần cứng, nạp firmware, xác minh trên OS và test âm thanh Hi-Res 96kHz/24-bit.

---

## Nội dung

1. [Danh sách linh kiện](#danh-sách-linh-kiện)
2. [Sơ đồ nối dây](#sơ-đồ-nối-dây)
3. [Hướng dẫn nạp Firmware](#hướng-dẫn-nạp-firmware)
4. [Xác minh trên Hệ Điều Hành](#xác-minh-trên-hệ-điều-hành)
5. [Kiểm tra và Test Âm thanh](#kiểm-tra-và-test-âm-thanh)
6. [Khắc phục sự cố](#khắc-phục-sự-cố)

---

## Danh sách linh kiện

Để hoàn thành hệ thống Pico 2 W USB DAC, bạn cần chuẩn bị:

### Phần cứng chính
- **1x Raspberry Pi Pico 2 W** (RP2350, 520 KB SRAM)
- **1x I2S DAC Module** (PCM5102A, MAX98357A, CS4344, hoặc tương đương)
- **1x Cáp USB Micro-B** (để nạp firmware và kết nối với máy tính)

### Dây nối
- **4x đầu cắm nữ DuPont** hoặc **lõi dây mỏng** để kết nối I2S
- **2x đầu cắm nữ DuPont** cho GND và VCC (3V3 hoặc 5V tùy theo DAC)

### Công cụ
- **Máy tính Windows / macOS / Linux**
- **Software để phát nhạc Hi-Res** (foobar2000, Audacity, Spotify, YouTube Music, v.v.)
- **Tai nghe hoặc loa khuếch đại** để nghe đầu ra I2S

---

## Sơ đồ nối dây

### Bảng Pinout Pico 2 W → I2S DAC

| Chân Pico 2 W | Tín hiệu | Chân DAC | Mô tả |
|:---|:---|:---|:---|
| **GP2** | **BCLK** (Bit Clock) | SCK / BCLK | Xung nhịp bit dữ liệu, 96 kHz × 32 bit × 2 kênh = 6.144 MHz |
| **GP3** | **WCLK** (Word Clock / LRCLK) | WS / LRCK | Xác định kênh trái (L) và phải (R), 96 kHz |
| **GP8** | **DOUT** (Data Out) | DIN / SD | Dữ liệu PCM 24-bit stereo |
| **GP9** | **MCLK** (Master Clock) | MCLK | Xung tham chiếu cho DAC, 24.576 MHz (256 × 96 kHz) |
| **GND** | Ground | GND | Đất chung |
| **3V3 hoặc 5V** | Cấp nguồn | VCC / VDD | Nguồn cho DAC (kiểm tra spec DAC của bạn) |

### Sơ đồ kết nối trực quan

```
Pico 2 W (RP2350)          |          I2S DAC Module
                           |
GP2 (BCLK) ────────────────┼────────> SCK / BCLK
GP3 (WCLK) ────────────────┼────────> WS / LRCK
GP8 (DOUT) ────────────────┼────────> DIN / SD
GP9 (MCLK) ────────────────┼────────> MCLK
                           |
GND ───────────────────────┼────────> GND
3V3 hoặc 5V ───────────────┼────────> VCC / VDD
                           |
[USB Micro-B] ◄───────────┼ Kết nối với PC
                           |
Serial1 (GP0/GP1)──────────┼─────────> USB-UART (debug log tùy chọn)
```

### Lưu ý kết nối
- **Sử dụng dây bao vòm (shielded cable)** cho các tín hiệu I2S để giảm nhiễu điện từ.
- **Giữ khoảng cách xa nguồn điện cao tần** để tránh can nhiễu tín hiệu audio.
- **Kiểm tra điện áp hoạt động của DAC** trước khi cấp nguồn (thường 3V3 hoặc 5V).
- **Không cắm/rút cáp khi đang phát nhạc** để tránh làm hỏng DAC hoặc loa.

---

## Hướng dẫn nạp Firmware

### Bước 1: Chuẩn bị file firmware

File firmware được biên dịch thành định dạng `.uf2` (UF2 Boot Loader Format) tại:
```
.pio/build/pico2w_usb_dac/firmware.uf2
```

Đảm bảo file này tồn tại sau khi biên dịch thành công:
```bash
pio run -e pico2w_usb_dac
```

### Bước 2: Chế độ BOOTSEL

1. **Cắm cáp USB** vào Pico 2 W nhưng **không kết nối tới PC chưa**.
2. **Giữ nút BOOTSEL** (nút nhỏ trên mặt trên Pico 2 W).
3. **Cắm Pico 2 W vào cổng USB của PC** trong khi vẫn giữ BOOTSEL.
4. **Thả nút BOOTSEL** sau khi cắm.
5. Pico 2 W sẽ xuất hiện như một **ổ đĩa USB** với tên `RPI-RP2` hoặc `BOOTSEL` (tuỳ hệ điều hành).

### Bước 3: Nạp file `.uf2`

#### **Windows:**

1. **Mở File Explorer** và tìm ổ đĩa `RPI-RP2`.
2. **Kéo và thả** file `firmware.uf2` vào thư mục gốc của ổ đĩa.
3. Hoặc **Copy (Ctrl+C)** → **Paste (Ctrl+V)** file vào ổ đĩa.
4. Ổ đĩa sẽ **tự động ngắt kết nối** và Pico 2 W sẽ **khởi động lại** với firmware mới.

#### **macOS:**

1. **Mở Finder** và tìm ổ đĩa `RPI-RP2`.
2. **Kéo và thả** file `firmware.uf2` vào ổ đĩa.
3. Hoặc dùng lệnh:
   ```bash
   cp firmware.uf2 /Volumes/RPI-RP2/
   ```
4. Pico 2 W sẽ tự động khởi động lại.

#### **Linux:**

1. **Tìm mount point** của ổ đĩa:
   ```bash
   lsblk
   # hoặc
   mount | grep -i rpi
   ```
   Thường là `/media/username/RPI-RP2` hoặc `/mnt/RPI-RP2`.

2. **Copy file vào ổ đĩa:**
   ```bash
   cp firmware.uf2 /media/username/RPI-RP2/
   ```
   hoặc
   ```bash
   sudo cp firmware.uf2 /mnt/RPI-RP2/
   ```

3. **Unmount ổ đĩa** (tùy chọn, Pico sẽ tự động restart):
   ```bash
   umount /media/username/RPI-RP2
   ```

### Bước 4: Xác nhận nạp thành công

- Pico 2 W sẽ **tắt đèn LED hoặc nhấp nháy** khi khởi động.
- **Kết nối cáp USB** (nếu chưa) với PC để kiểm tra trong phần [Xác minh trên Hệ Điều Hành](#xác-minh-trên-hệ-điều-hành).

---

## Xác minh trên Hệ Điều Hành

Sau khi nạp firmware thành công, kiểm tra xem PC nhận diện Pico 2 W là **Sound Card UAC2 96kHz / 24-bit Stereo**.

### Windows

1. **Cắm Pico 2 W** vào cổng USB của PC.
2. **Nhấn Windows + I** để mở **Settings**.
3. Đi tới **System** → **Sound**.
4. Cuộn xuống mục **Advanced** → **Volume mixer**.
5. Tìm thiết bị **"Pico USB Audio"** hoặc **"Audio Class 2.0"**.
6. **Nhấp chuột phải** lên thiết bị → **Properties**.
7. Kiểm tra tab **Advanced** để xem định dạng được hỗ trợ:
   - Tìm **96000 Hz, 24-bit Stereo** hoặc tương tự.

**Lệnh PowerShell để kiểm tra (tùy chọn):**
```powershell
Get-AudioDevice | Where-Object {$_.Name -like "*Pico*"} | Format-List
```

### macOS

1. **Cắm Pico 2 W** vào cổng USB của Mac.
2. Mở **System Preferences** → **Sound**.
3. Chọn tab **Output**.
4. Tìm **"Pico USB Audio"** hoặc **"USB Audio Class 2.0"** trong danh sách.
5. **Nhấp vào** để chọn làm thiết bị đầu ra mặc định.
6. Nếu có nút **Options** hoặc **Format**, kiểm tra:
   - Tốc độ mẫu: **96000 Hz**
   - Độ sâu bit: **24-bit**

**Lệnh Terminal để kiểm tra (tùy chọn):**
```bash
system_profiler SPAudioDataType | grep -A 20 "Pico"
```

### Linux

1. **Cắm Pico 2 W** vào cổng USB.
2. **Kiểm tra bằng lệnh:**

   **Với ALSA:**
   ```bash
   arecord -l
   # hoặc
   cat /proc/asound/cards
   ```
   Tìm card có tên chứa "Pico" hoặc "USB Audio".

   **Với PulseAudio:**
   ```bash
   pactl list cards
   # hoặc
   pacmd list-cards | grep -A 20 "Pico\|USB"
   ```

   **Với PipeWire (mới hơn):**
   ```bash
   pw-cli list | grep -A 10 "Pico"
   ```

3. **Xem chi tiết định dạng hỗ trợ:**
   ```bash
   aplay -l
   aplay -L
   # Chọn thiết bị Pico và kiểm tra format được hỗ trợ
   ```

4. **Hoặc dùng PulseAudio Volume Control (pavucontrol):**
   ```bash
   sudo apt install pavucontrol
   pavucontrol
   ```
   Chọn tab **Output Devices** và tìm Pico.

---

## Kiểm tra và Test Âm thanh

### Bước 1: Kiểm tra kết nối vật lý

- [ ] **Kiểm tra tất cả dây nối** I2S (BCLK, WCLK, DOUT, MCLK, GND, VCC).
- [ ] **Đảm bảo không có dây nối bị ngược** hoặc hở.
- [ ] **Kiểm tra DAC có tín hiệu cấp nguồn** (kiểm tra đèn LED trên DAC nếu có).
- [ ] **Tai nghe / loa khuếch đại được kết nối đúng** với đầu ra của DAC.

### Bước 2: Phát âm thanh test

1. **Chọn Pico USB Audio làm thiết bị đầu ra mặc định** trên OS.
2. **Mở phần mềm phát nhạc:**
   - **foobar2000** (Windows) - hỗ trợ FLAC Hi-Res
   - **Audacity** (Windows/macOS/Linux) - tạo tiếng báo thử nghiệm
   - **Spotify / YouTube Music / Apple Music** - streaming thông thường
   - **VLC Player** - phát các định dạng khác nhau

3. **Chọn file âm thanh:**
   - **Hi-Res FLAC 96kHz/24-bit** (nếu có)
   - **Tệp WAV 96kHz/24-bit**
   - Hoặc bất kỳ file mp3/aac thông thường

4. **Nhấn Play** và nghe qua tai nghe / loa.

### Bước 3: Tiêu chí kiểm tra chất lượng âm thanh

#### **Kiểm tra chung:**
- [ ] **Âm thanh được phát ra** - có tín hiệu audio từ I2S DAC.
- [ ] **Dải tần số đầy đặn** - âm bass, mid, treble đều có mặt.
- [ ] **Không có tiếng xì nền (hiss)** hoặc nhiễu cao tần rõ rệt.
- [ ] **Âm thanh sạch và rõ ràng** - không bị bóp méo hoặc méo tiếng.

#### **Kiểm tra tính ổn định:**
- [ ] **Bấm Play / Pause liên tiếp** - không có tiếng "bốp/pop/click" giật.
- [ ] **Tua / Seek trong bài hát** - âm thanh tiếp tục mượt mà, không bị ghẫn.
- [ ] **Chuyển bài hát** - quá trình chuyển tiếp êm dịu, không có bước nhảy tần số.

#### **Kiểm tra khả năng phục hồi:**
- [ ] **Ngắt cáp USB rồi cắm lại** - hệ thống tự phục hồi bình thường.
- [ ] **Để PC sleep rồi tỉnh dậy** - âm thanh tiếp tục từ nơi tạm dừng.
- [ ] **Vô hiệu hóa / kích hoạt lại thiết bị audio** - không gây sốc xung hoặc tạp âm.

### Bước 4: Test trong các tình huống khác nhau

#### **Test 1: Tín hiệu đơn tần (Pure Tone Test)**
- Tạo hoặc tải tệp sinh ra **1 kHz, 24-bit, 96kHz**.
- Nghe để kiểm tra:
  - Tính sạch của tín hiệu (không bị gợn sóng hay biến dạng).
  - Tần số chính xác (nếu có thiết bị đo tần).

#### **Test 2: Quét tần số (Frequency Sweep)**
- Phát từ **20 Hz đến 20 kHz** trong 30 giây.
- Nghe để kiểm tra:
  - Phần cứng xử lý đều trên toàn dải.
  - Không có "điểm yếu" hoặc khe hở tần số.

#### **Test 3: Nhạc chuẩn Hi-Fi**
- Phát các album **Hi-Res FLAC** hoặc **DSD** (nếu được hỗ trợ):
  - Ví dụ: Album jazz, classical, audiophile-grade recordings.
- Nghe để kiểm tra:
  - Chi tiết và tách biệt âm thanh (stereo imaging).
  - Động lực (dynamics) và tỷ lệ tín hiệu/nhiễu (SNR).

#### **Test 4: Nhạc thường (Casual Testing)**
- Phát **Spotify, YouTube Music, hoặc podcast** thông thường.
- Kiểm tra:
  - Hệ thống có hoạt động ổn định với các định dạng khác nhau không.
  - Có độ trễ âm thanh (latency) hay không.

### Bước 5: Kiểm tra Debug Log (tùy chọn)

Nếu bạn có bộ USB-UART converter kết nối với **GP0 (TX) / GP1 (RX)** của Pico 2 W:

1. **Cấu hình cổng serial** ở tốc độ **115200 baud**.
2. **Mở terminal (PuTTY, minicom, hoặc Arduino IDE Serial Monitor)**.
3. **Quan sát log**:
   ```
   =========================================
   Pico 2 W (RP2350) Hi-Res USB DAC Initializing...
   Target Audio: 96000 Hz, 24-bit, 2 Channels
   [OK] I2S DAC initialized successfully.
   [OK] USB Audio UAC2 initialized successfully.
   [READY] USB DAC is running. Connect to PC via USB!
   =========================================
   [USB EVENT] Device mounted to Host PC.
   [STATUS] Stream throughput: 576 bytes/5s, Total: 576 KB
   [STATUS] Stream throughput: 576 bytes/5s, Total: 1152 KB
   ...
   ```
4. **Kiểm tra**:
   - Không có thông báo `[ERROR]`.
   - Thông tin throughput ổn định và tăng dần (cho thấy dòng dữ liệu hoạt động).
   - Khi tạm dừng, throughput giảm về 0.

---

## Khắc phục sự cố

### Vấn đề: Pico 2 W không xuất hiện trên danh sách thiết bị âm thanh

**Nguyên nhân có thể:**
1. Firmware chưa được nạp đúng.
2. Cáp USB bị hỏng hoặc không kết nối đầy đủ.
3. Driver USB Audio bị cũ hoặc thiếu.

**Cách khắc phục:**
- [ ] **Kiểm tra lại nạp firmware:**
  ```bash
  pio run -e pico2w_usb_dac -t upload
  # hoặc nạp thủ công theo hướng dẫn BOOTSEL
  ```
- [ ] **Thay cáp USB** bằng cáp khác hoặc cổng USB khác trên PC.
- [ ] **Windows**: Cập nhật driver USB Audio Class 2.0 từ Windows Update hoặc trang nhà sản xuất.
- [ ] **macOS**: Cập nhật macOS lên phiên bản mới nhất.
- [ ] **Linux**: Cập nhật kernel và kiểm tra log USB:
  ```bash
  dmesg | tail -20
  ```

### Vấn đề: Âm thanh bị méo hoặc có tạp âm

**Nguyên nhân có thể:**
1. Dây nối I2S bị nhiễu từ trường.
2. Cấp nguồn không ổn định hoặc không đủ.
3. Tần số mẫu không khớp giữa USB và I2S DAC.

**Cách khắc phục:**
- [ ] **Kiểm tra dây nối**: Sử dụng dây bao vòm (shielded cable) và tách ra khỏi nguồn điện.
- [ ] **Kiểm tra cấp nguồn**:
  - DAC có được cấp điện đúng không (3V3 hoặc 5V)?
  - Có đủ dòng không (thường 50-200 mA)?
  - Có sử dụng tụ lọc / stabilizer?
- [ ] **Kiểm tra tần số mẫu**: Đảm bảo OS và DAC đều cấu hình **96kHz, 24-bit**.

### Vấn đề: Có tiếng "pop" hoặc "click" khi bấm Play / Pause

**Nguyên nhân có thể:**
1. Buffer không được nạp đầy trước khi phát.
2. Rút / cắm cáp USB khi đang phát.

**Cách khắc phục:**
- [ ] **Tránh cắm/rút cáp USB** khi đang phát nhạc.
- [ ] **Kiểm tra log serial** để xem có underflow hay không.
- [ ] **Tăng kích thước buffer** trong firmware nếu cần:
  ```cpp
  usb_cfg.fifo_packets = 64;  // Tăng từ 48 lên 64
  ```

### Vấn đề: Âm thanh bị lag hoặc trễ (latency)

**Nguyên nhân có thể:**
1. Buffer quá lớn.
2. CPU bị quá tải.
3. Kết nối USB bị suyếp.

**Cách khắc phục:**
- [ ] **Giảm kích thước buffer** (đổi lại `fifo_packets = 48`).
- [ ] **Đóng các ứng dụng không cần thiết** trên PC.
- [ ] **Kiểm tra kết nối USB**: Sử dụng cổng USB khác hoặc hub USB có nguồn riêng.

### Vấn đề: Log serial hiển thị `[ERROR] usb.begin() failed!`

**Nguyên nhân:**
Thư viện `arduino-audio-tools` không được cấu hình đúng.

**Cách khắc phục:**
- [ ] **Kiểm tra `platformio.ini`**: Đảm bảo `lib_deps` chứa đúng phiên bản:
  ```ini
  lib_deps =
      https://github.com/pschatzmann/arduino-audio-tools.git#v1.2.6
      adafruit/Adafruit TinyUSB Library
  ```
- [ ] **Xóa thư mục `.pio` và biên dịch lại:**
  ```bash
  rm -rf .pio
  pio run -e pico2w_usb_dac
  ```

---

## Các bước tiếp theo

Sau khi hoàn thành bài test này, bạn có thể:

1. **Tích hợp Wi-Fi Streaming**: Thêm hỗ trợ streaming âm nhạc từ mạng Wi-Fi khi USB không kết nối.
2. **Thêm State Machine**: Tự động chuyển đổi giữa chế độ USB DAC và Wi-Fi Radio.
3. **Tinh chỉnh audio**: Cải thiện độ sạch tín hiệu, thêm EQ, hoặc hỗ trợ định dạng DSD.
4. **Đóng gói phần cứng**: Thiết kế hộp chứa, cố định dây nối, thêm các nút điều khiển.

---

## Tài liệu tham khảo

- [RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
- [Pico 2 W Hardware Design](https://datasheets.raspberrypi.com/pico-2w/pico-2-w-datasheet.pdf)
- [USB Audio Class 2.0 Specification](https://www.usb.org/documents)
- [arduino-audio-tools Documentation](https://github.com/pschatzmann/arduino-audio-tools)
- [Adafruit TinyUSB Library](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)
- [Tài liệu thiết kế firmware](./superpowers/specs/2026-11-10-pico2w-usb-dac-design.md)

---

## Liên hệ & Hỗ trợ

Nếu gặp vấn đề không được liệt kê ở trên:

1. **Kiểm tra lại các bước** trong tài liệu này.
2. **Xem log serial debug** từ Pico 2 W.
3. **Kiểm tra kết nối vật lý** tất cả dây nối và cấp nguồn.
4. **Tham khảo các diễn đàn**: Raspberry Pi Forums, Arduino Community, hoặc AudioTools GitHub Issues.

---

**Ghi chú:** Tài liệu này được cập nhật cho firmware Pico 2 W USB DAC phiên bản v1.0 (2026-11-10). Nếu bạn sử dụng phiên bản firmware khác, một số bước có thể khác biệt.
