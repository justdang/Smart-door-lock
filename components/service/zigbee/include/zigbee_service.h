#ifndef ZIGBEE_SERVICE_H
#define ZIGBEE_SERVICE_H
#include "zigbee_driver.h"
#include "zigbee_low_level.h" //có thể thay đổi theo Thành
#include <stdint.h>

typedef enum {
    ZIGBEE_SERVICE_OK               =  0,
    ZIGBEE_SERVICE_ERR_NOT_INIT     = -1,   
    ZIGBEE_SERVICE_ERR_PARAM        = -2,   
    ZIGBEE_SERVICE_ERR_DRIVER       = -3,   
    ZIGBEE_SERVICE_ERR_PAIRING      = -4,   
    ZIGBEE_SERVICE_ERR_OFFLINE      = -5,   
} zigbee_service_err_t;

typedef enum {
    ZIGBEE_PAIR_SUCCESS  = 0x00,    // join hub thành công
    ZIGBEE_PAIR_TIMEOUT  = 0x01,    // timeout vì không thấy hub
    ZIGBEE_PAIR_FAILED   = 0x02,    // join hub thất bại
} zigbee_pair_status_t;

typedef struct {
    zigbee_lock_state_t lock;       // LOCKED or UNLOCKED
    zigbee_door_state_t door;       // OPEN or CLOSED
} zigbee_device_state_t;
//check driver

//init
zigbee_service_err_t zigbee_service_init(const uint8_t aes_key[ZIGBEE_AES_KEY_SIZE]); //ZIGBEE_AES_KEY_SIZE ở trong zigbee_driver.h
//pairing 
zigbee_service_err_t zigbee_service_pair(uint32_t timeout_ms, zigbee_pair_status_t* out_status);
zigbee_service_err_t zigbee_service_isConnected(bool* out_online);
zigbee_service_err_t zigbee_service_unpair(void);
//Sending states
zigbee_service_err_t zigbee_service_send_lock_state(zigbee_lock_state_t state);
zigbee_service_err_t zigbee_service_send_door_state(zigbee_door_state_t state);
//receiving commands
void service_zigbee_on_receive(const zigbee_frame_t* frame);

#endif