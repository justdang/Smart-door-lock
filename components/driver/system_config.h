#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

// --- 1. CẤU HÌNH SERVO ---
#define PIN_SERVO           18

// --- 2. CẤU HÌNH KEYPAD 4x4 
#define PIN_KEYPAD_R1       19
#define PIN_KEYPAD_R2       21
#define PIN_KEYPAD_R3       33  
#define PIN_KEYPAD_R4       23
#define PIN_KEYPAD_C1       25
#define PIN_KEYPAD_C2       26
#define PIN_KEYPAD_C3       27
#define PIN_KEYPAD_C4       32

// --- 3. CẤU HÌNH RFID (SPI) ---
#define PIN_RFID_MISO       12
#define PIN_RFID_MOSI       13
#define PIN_RFID_CLK        14
#define PIN_RFID_CS         15
#define PIN_RFID_RST        4

// --- 4. CẤU HÌNH MÀN HÌNH TFT --- 
#define PIN_DISPLAY_CS      2   
#define PIN_DISPLAY_DC      0   
#define PIN_DISPLAY_RST     34  
#define PIN_DISPLAY_BCKL    16 

// --- 5. CẤU HÌNH MODULE RTC (I2C) ---
#define PIN_RTC_SDA         22  
#define PIN_RTC_SCL         5   

// --- 6. CẤU HÌNH MODULE ZIGBEE (UART) ---
#define PIN_ZIGBEE_TX       17  
#define PIN_ZIGBEE_RX       16  

#endif // SYSTEM_CONFIG_H