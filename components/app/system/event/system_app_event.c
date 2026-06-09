#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "system_app_event.h"
#include "freertos/FreeRTOS.h" 
#include "esp_log.h"  

#define EVENT_QUEUE_SIZE 16

static AppEventMessage s_queue[EVENT_QUEUE_SIZE];
static uint8_t         s_head  = 0;
static uint8_t         s_tail  = 0;
static uint8_t         s_count = 0;

void system_event_init(void){
    memset(s_queue, 0, sizeof(s_queue));
    s_head = 0;
    s_tail = 0;
    s_count = 0;
}

bool system_event_post(AppEventMessage* msg){
    if(msg == NULL){
        ESP_LOGW("SYS_EVT", "post: NULL msg");
        return false;
    } // can nhac them thanh 1 event hay error -> ERROR/EVENT ma message rong
    if(s_count >= EVENT_QUEUE_SIZE){
        ESP_LOGW("SYS_EVT", "post: queue full");
        return false;
    } // full hang doi 
    s_queue[s_tail] = *msg; // copy msg vao queue
    s_tail = (s_tail + 1) % EVENT_QUEUE_SIZE; // cap nhat tail
    s_count++;
    return true;
}

bool system_event_get(AppEventMessage* msg){
    if(msg == NULL){
        ESP_LOGW("SYS_EVT", "get: NULL msg");
        return false;
    }
    if(s_count == 0){
        ESP_LOGW("SYS_EVT", "get: queue empty");
        return false; // queue rong
    }
    *msg = s_queue[s_head]; // copy msg tu queue
    s_head = (s_head + 1) % EVENT_QUEUE_SIZE; // cap nhat head
    s_count--;
    return true;
}

bool system_pending(void) return s_count > 0; // true neu con event trong queue



