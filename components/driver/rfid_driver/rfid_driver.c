#include "rfid_driver.h"
#include "system_config.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>

static spi_device_handle_t spi_handle;

static void RFID_Init_HW(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_RFID_MISO,
        .mosi_io_num = PIN_RFID_MOSI,
        .sclk_io_num = PIN_RFID_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5 * 1000 * 1000, // 5MHz
        .mode = 0,                         // Mode 0 cho RC522
        .spics_io_num = PIN_RFID_CS,
        .queue_size = 7
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);

    // TODO: Khởi tạo thanh ghi module MFRC522 (PCD_Init)
}

static bool RFID_IsCardPresent_HW(void) {
    // TODO: Gửi lệnh PICC_RequestA qua SPI.
    // Nếu có thẻ ở gần, return true.
    return false; 
}

static bool RFID_ReadUID_HW(rfid_uid_t *uid_user) {
    // TODO: Gửi lệnh PICC_Anticoll qua SPI để đọc UID.
    // Nếu thành công:
    // memcpy(uid_user->uid, raw_uid_buffer, length);
    // uid_user->length = length;
    // return true;
    return false;
}

static void RFID_Halt_HW(void) {
    // TODO: Gửi lệnh PICC_HaltA qua SPI để thẻ ngủ, tránh đọc lặp lại.
}

const RFID_Driver_t ESP32_RFID_Driver = {
    .Init          = RFID_Init_HW,
    .IsCardPresent = RFID_IsCardPresent_HW,
    .ReadUID       = RFID_ReadUID_HW,
    .Halt          = RFID_Halt_HW
};
