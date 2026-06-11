#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

// ==========================================
// 1. CẤU HÌNH SERVO
// ==========================================
#define PIN_SERVO           18
#define SERVO_LOCK_ANGLE    0    
#define SERVO_UNLOCK_ANGLE  90   

// ==========================================
// 2. CẤU HÌNH KEYPAD 4x4 
// ==========================================
#define PIN_KEYPAD_R1       19
#define PIN_KEYPAD_R2       21
#define PIN_KEYPAD_R3       33  
#define PIN_KEYPAD_R4       23
#define PIN_KEYPAD_C1       25
#define PIN_KEYPAD_C2       26
#define PIN_KEYPAD_C3       27
#define PIN_KEYPAD_C4       32

// ==========================================
// 3. CẤU HÌNH BUS SPI CHUNG (RFID & TFT DISPLAY)
// ==========================================
#define PIN_SPI_MISO        12  // Nối tới MISO của RC522 và SDO(MISO) của TFT
#define PIN_SPI_MOSI        13  // Nối tới MOSI của RC522 và SDI(MOSI) của TFT
#define PIN_SPI_CLK         14  // Nối tới SCK của RC522 và SCL(CLK) của TFT

// Chân chọn chip riêng biệt (CS) để phân loại thiết bị trên Bus SPI
#define PIN_RFID_CS         15
#define PIN_RFID_RST        4

#define PIN_DISPLAY_CS      2   
#define PIN_DISPLAY_DC      16  
#define PIN_DISPLAY_RST     0   // Bổ sung chân Reset riêng cho màn hình TFT (hoặc nối lên 3.3V/nối chung chân 4)

// ==========================================
// 4. CẤU HÌNH MODULE RTC (I2C)
// ==========================================
#define PIN_RTC_SDA         22  
#define PIN_RTC_SCL         5   

// ==========================================
// 5. CẤU HÌNH MODULE ZIGBEE (UART)
// ==========================================
#define PIN_ZIGBEE_TX       17  
#define PIN_ZIGBEE_RX       34 

#endif // SYSTEM_CONFIG_H