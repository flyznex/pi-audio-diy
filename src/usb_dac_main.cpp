#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <atomic>
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
// Con trỏ tới driver I2S cụ thể (RP2040): i2s.driver() chỉ trả về I2SDriverBase*,
// loại cơ sở không khai báo flush()/getOverUnderflow(). Driver thật sự trên RP2040
// luôn là I2SDriverRP2040 nên ép kiểu 1 lần ở setup() là an toàn.
static I2SDriverRP2040* i2s_driver = nullptr;

// Cờ sự kiện USB: callback TinyUSB chạy trong ngữ cảnh ngắt (IRQ), nên chỉ được
// phép ghi cờ ở đây — mọi việc tốn thời gian (Serial1, flush DMA) phải dời sang
// loop() để tránh làm trễ/stall ngắt.
enum UsbEvent : uint32_t {
    USB_EVT_MOUNTED   = 1u << 0,
    USB_EVT_UNMOUNTED = 1u << 1,
    USB_EVT_SUSPENDED = 1u << 2,
    USB_EVT_RESUMED   = 1u << 3,
};
static std::atomic<uint32_t> usb_event_flags{0};

// Biến thống kê trạng thái
static uint32_t last_stat_ms = 0;
static uint32_t bytes_copied_total = 0;

// Callback khi máy tính kết nối và nhận diện thành công (chạy trong IRQ context)
void tud_mount_cb(void) {
    usb_event_flags.fetch_or(USB_EVT_MOUNTED, std::memory_order_relaxed);
}

// Callback khi rút cáp hoặc ngắt kết nối USB (chạy trong IRQ context)
void tud_umount_cb(void) {
    usb_event_flags.fetch_or(USB_EVT_UNMOUNTED, std::memory_order_relaxed);
}

// Callback khi máy tính vào chế độ Sleep / Suspend (chạy trong IRQ context)
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    usb_event_flags.fetch_or(USB_EVT_SUSPENDED, std::memory_order_relaxed);
}

// Callback khi máy tính thức dậy (Resume) (chạy trong IRQ context)
void tud_resume_cb(void) {
    usb_event_flags.fetch_or(USB_EVT_RESUMED, std::memory_order_relaxed);
}

// Xử lý các sự kiện USB đã ghi nhận ở ISR, nhưng thực thi an toàn trong loop()
static void process_usb_events() {
    uint32_t events = usb_event_flags.exchange(0, std::memory_order_relaxed);
    if (events == 0) return;

    if (events & USB_EVT_MOUNTED) {
        Serial1.println("[USB EVENT] Device mounted to Host PC.");
    }
    if (events & USB_EVT_UNMOUNTED) {
        Serial1.println("[USB EVENT] Device unmounted.");
        if (i2s_driver) i2s_driver->flush();
    }
    if (events & USB_EVT_SUSPENDED) {
        Serial1.println("[USB EVENT] USB suspended (Host sleep/idle).");
        if (i2s_driver) i2s_driver->flush();
    }
    if (events & USB_EVT_RESUMED) {
        Serial1.println("[USB EVENT] USB resumed.");
    }
}

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
    // Lưu lại con trỏ driver cụ thể để dùng flush()/getOverUnderflow() sau này.
    i2s_driver = static_cast<I2SDriverRP2040*>(i2s.driver());

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
    // Xử lý các sự kiện USB mount/unmount/suspend/resume ghi nhận từ ISR
    process_usb_events();

    // Luân chuyển dữ liệu từ USB sang I2S
    size_t copied = copier.copy();
    if (copied > 0) {
        bytes_copied_total += copied;
    }

    // Thống kê nhịp tim định kỳ mỗi 5 giây ra Serial1
    uint32_t now = millis();
    if (now - last_stat_ms >= 5000) {
        last_stat_ms = now;
        bool underflow = i2s_driver != nullptr && i2s_driver->getOverUnderflow();
        Serial1.printf("[STATUS] Stream throughput: %u bytes/5s, Total: %u KB, Underflow: %s\n",
                       bytes_copied_total, (bytes_copied_total / 1024),
                       underflow ? "YES" : "no");
        bytes_copied_total = 0;
    }
}
