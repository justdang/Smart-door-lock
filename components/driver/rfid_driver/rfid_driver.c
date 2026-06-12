#include "rfid_driver.h"
#include "system_config.h"
<<<<<<< Updated upstream
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
=======
#include "driver/spi_master.h" //low level
#include "driver/gpio.h" //low level
>>>>>>> Stashed changes
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG_RFID "RFID_DRIVER"

// Các thanh ghi MFRC522 cơ bản
#define CommandReg    0x01
#define ComIEnReg     0x02
#define DivIEnReg     0x03
#define ComIrqReg     0x04
#define DivIrqReg     0x05
#define ErrorReg      0x06
#define Status2Reg    0x08
#define FIFODataReg   0x09
#define FIFOLevelReg  0x0A
#define WaterLevelReg 0x0B
#define ControlReg    0x0C
#define BitFramingReg 0x0D
#define CollReg       0x0E
#define ModeReg       0x11
#define TxModeReg     0x12
#define RxModeReg     0x13
#define TxControlReg  0x14
#define TxASKReg      0x15
#define TModeReg      0x2A
#define TPrescalerReg 0x2B
#define TReloadRegH   0x2C
#define TReloadRegL   0x2D

// Lệnh MFRC522
#define PCD_IDLE      0x00
#define PCD_RCV_CMD   0x08
#define PCD_TRANSCEIVE 0x0C
#define PCD_RESETPHASE 0x0F

// Lệnh ISO14443 cho Thẻ Mifare
#define PICC_REQIDL   0x26
#define PICC_ANTICOLL 0x93
#define PICC_HALT     0x50

static spi_device_handle_t spi_handle;

// Hàm Low-level ghi vào thanh ghi MFRC522
static void mfrc522_write_reg(uint8_t addr, uint8_t val) {
    uint8_t tx_buf[2] = { (addr << 1) & 0x7E, val };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buf,
    };
    spi_device_transmit(spi_handle, &t);
}

// Hàm Low-level đọc từ thanh ghi MFRC522
static uint8_t mfrc522_read_reg(uint8_t addr) {
    uint8_t tx_buf[2] = { ((addr << 1) & 0x7E) | 0x80, 0x00 };
    uint8_t rx_buf[2] = {0};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };
    spi_device_transmit(spi_handle, &t);
    return rx_buf[1];
}

static void mfrc522_set_bit_mask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = mfrc522_read_reg(reg);
    mfrc522_write_reg(reg, tmp | mask);
}

static void mfrc522_clear_bit_mask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = mfrc522_read_reg(reg);
    mfrc522_write_reg(reg, tmp & (~mask));
}

// Hàm giao tiếp dữ liệu với thẻ
static bool mfrc522_to_card(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint32_t *backLen) {
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    
    if (command == PCD_TRANSCEIVE) {
        irqEn = 0x77;
        waitIRq = 0x30;
    }
    
    mfrc522_write_reg(ComIEnReg, irqEn | 0x80);
    mfrc522_clear_bit_mask(ComIrqReg, 0x80);
    mfrc522_set_bit_mask(FIFOLevelReg, 0x80); // Flush FIFO
    mfrc522_write_reg(CommandReg, PCD_IDLE);

    for (int i = 0; i < sendLen; i++) {
        mfrc522_write_reg(FIFODataReg, sendData[i]);
    }

    mfrc522_write_reg(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        mfrc522_set_bit_mask(BitFramingReg, 0x80); // StartSend
    }

    // Đợi tối đa ~25ms
    int i = 2000;
    uint8_t n;
    do {
        n = mfrc522_read_reg(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    mfrc522_clear_bit_mask(BitFramingReg, 0x80);

    if (i == 0) return false;
    if (mfrc522_read_reg(ErrorReg) & 0x1B) return false; // Thừa/thiếu/lỗi bit hoặc va chạm

    if (n & waitIRq) {
        if (backData && backLen) {
            uint8_t nn = mfrc522_read_reg(FIFOLevelReg);
            uint8_t lastBits = mfrc522_read_reg(ControlReg) & 0x07;
            if (lastBits) {
                *backLen = (nn - 1) * 8 + lastBits;
            } else {
                *backLen = nn * 8;
            }
            if (nn == 0) nn = 1;
            for (int j = 0; j < nn; j++) {
                backData[j] = mfrc522_read_reg(FIFODataReg);
            }
        }
        return true;
    }
    return false;
}

static void RFID_Init_HW(void) {
    // Khởi tạo chân RST bằng GPIO config độc lập
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_RFID_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    gpio_set_level(PIN_RFID_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // SỬA ĐỔI: Khởi tạo Bus SPI sử dụng các hằng số chân chung mới để chia sẻ với Màn hình TFT
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_SPI_MISO, // Thay đổi từ PIN_RFID_MISO
        .mosi_io_num = PIN_SPI_MOSI, // Thay đổi từ PIN_RFID_MOSI
        .sclk_io_num = PIN_SPI_CLK,  // Thay đổi từ PIN_RFID_CLK
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000, // 4MHz ổn định cho RC522
        .mode = 0,                         
        .spics_io_num = PIN_RFID_CS,       // Giữ nguyên chân CS riêng của RFID
        .queue_size = 7
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);

    // Thực hiện Hardware Reset MFRC522 qua thanh ghi (PCD_Init)
    gpio_set_level(PIN_RFID_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RFID_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    mfrc522_write_reg(CommandReg, PCD_RESETPHASE);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Cấu hình các bộ định thời Timer & Chế độ Anten phát xạ sóng mang 13.56Mhz
    mfrc522_write_reg(TModeReg, 0x8D);
    mfrc522_write_reg(TPrescalerReg, 0x3E);
    mfrc522_write_reg(TReloadRegL, 30);
    mfrc522_write_reg(TReloadRegH, 0);
    mfrc522_write_reg(TxASKReg, 0x40);
    mfrc522_write_reg(ModeReg, 0x3D);

    // Bật Ăng-ten phát sóng
    uint8_t tx_ctrl = mfrc522_read_reg(TxControlReg);
    if (!(tx_ctrl & 0x03)) {
        mfrc522_set_bit_mask(TxControlReg, 0x03);
    }
    ESP_LOGI(TAG_RFID, "MFRC522 Hardware initialized successfully on shared SPI Bus.");
}

static bool RFID_IsCardPresent_HW(void) {
    uint8_t req_cmd = PICC_REQIDL;
    uint8_t back_data[2];
    uint32_t back_len;

    mfrc522_write_reg(BitFramingReg, 0x07); // Chuẩn hóa số bit lớp vật lý
    bool status = mfrc522_to_card(PCD_TRANSCEIVE, &req_cmd, 1, back_data, &back_len);
    
    if (status && back_len == 16) { // Thẻ Mifare hợp lệ sẽ trả lời bằng 2 byte ATQA (16 bits)
        return true;
    }
    return false;
}

static bool RFID_ReadUID_HW(rfid_uid_t *uid_user) {
    uint8_t send_data[2] = { PICC_ANTICOLL, 0x20 }; // Gửi mã chống va chạm tầng 1 mức dòng dữ liệu
    uint8_t back_data[5]; // Thẻ gửi lại 4 byte UID + 1 byte BCC check
    uint32_t back_len;

    mfrc522_write_reg(BitFramingReg, 0x00);
    bool status = mfrc522_to_card(PCD_TRANSCEIVE, send_data, 2, back_data, &back_len);

    if (status) {
        // Kiểm tra mã BCC XOR kiểm tra tính toàn vẹn
        uint8_t bcc = 0;
        for (int i = 0; i < 4; i++) {
            bcc ^= back_data[i];
        }
        if (bcc == back_data[4]) {
            memcpy(uid_user->uid, back_data, 4);
            uid_user->length = 4;
            return true;
        }
    }
    return false;
}

static void RFID_Halt_HW(void) {
    mfrc522_write_reg(CommandReg, PCD_IDLE);
    mfrc522_clear_bit_mask(Status2Reg, 0x08); // Tắt mã hóa Crypto1 của thẻ Mifare nếu có đang mở
}

const RFID_Driver_t ESP32_RFID_Driver = {
    .Init          = RFID_Init_HW,
    .IsCardPresent = RFID_IsCardPresent_HW,
    .ReadUID       = RFID_ReadUID_HW,
    .Halt          = RFID_Halt_HW
};