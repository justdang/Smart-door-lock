#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h" // có sẵn của espidf
#include "rfid_struct.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // giai thich thu vien nay 

#define MAX_PASSWORD_LENGTH 7 //6 ký tự và ký tự "\0"
#define MAX_ADMIN_PASSWORD_LENGTH 7
#define MAX_UIDS 10
#define MAX_PASSWORD 5
//ESP_LOGW: lệnh warning thông qua màn hình console sử dụng UART
//password
typedef struct {
    char    password[MAX_PASSWORD_LENGTH];
    bool    is_used;
} PasswordSlot;
static PasswordSlot user_passwords[MAX_PASSWORD];
static char admin_password[MAX_ADMIN_PASSWORD_LENGTH] = "111111"; //mật khẩu cho admin
//UID
static RFID_UID_t uid_storage[MAX_UIDS];
static uint32_t uid_count = 0;
//mutex
static SemaphoreHandle_t storage_mutex = NULL;|
//hàm nội bộ kiểm tra mật khẩu có valid không 
static bool _is_password_valid(const char* password) {
    if (password == NULL) return false;
    return (strlen(password) == MAX_PASSWORD_LENGTH);
}

//ok
void storage_init(void){
    storage_mutex = xSemaphoreCreateMutex();
    if (storage_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create storage mutex");
    }
    memset(user_passwords, 0, sizeof(user_passwords));
    strlcpy(user_passwords[0].password, "123456", MAX_PASSWORD_LENGTH); //mat khau mac dinh ban dau
    user_passwords[0].is_used = true;
    ESP_LOGI(TAG, "Storage initialized (RAM only).");
}
//mã nguồn password
//ok
bool storage_savePassword(char* password) {
  if (!_is_password_valid(password)) return false;
if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
for (uint8_t i = 0; i < MAX_PASSWORDS; i++){
    if(!user_passwords[i].is_used){
        strlcpy(user_password, password, MAX_PASSWORD_LENGTH);
        user_passwords[i],is_used = true;
        ESP_LOGI(TAG, "User password saved.");
        xSemaphoreGive(storage_mutex);
        return true;
    }
}
}
//get password
//ok
bool storage_getPassword(uint8_t index, char* out_password, size_t max_len) {
    if (out_password == NULL || max_len == 0) return false;
    if (index >= MAX_PASSWORDS) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
    if (!user_passwords[index].is_used) {
        // slot này trống, không có mật khẩu
        ESP_LOGW(TAG, "Password slot %d is empty.", index);
        xSemaphoreGive(storage_mutex);
        return false;
    }
    strlcpy(out_password, user_passwords[index].password, max_len);
    xSemaphoreGive(storage_mutex);
    return true;
}

bool storage_deletePassword(uint8_t index) {
    if (index >= MAX_PASSWORDS) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;

    if (!user_passwords[index].is_used) {
        ESP_LOGW(TAG, "Password slot %d already empty.", index);
        xSemaphoreGive(storage_mutex);
        return false;
    }

    memset(user_passwords[index].password, 0, MAX_PASSWORD_LENGTH);
    user_passwords[index].is_used = false;
    ESP_LOGI(TAG, "Password slot %d deleted.", index);

    xSemaphoreGive(storage_mutex);
    return true;
}
uint8_t storage_getPasswordCount(void) {
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return 0;

    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_PASSWORDS; i++) {
        if (user_passwords[i].is_used) count++;
    }

    xSemaphoreGive(storage_mutex);
    return count;
}

//rfid 
//ok
bool storage_addUID(RFID_UID_t* uid) {
    if (uid == NULL || uid_count >= MAX_UIDS) {
        ESP_LOGW(TAG, "Storage full or invalid UID");
        return false;
    }
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
    // 1. Kiểm tra xem UID này đã tồn tại chưa để tránh trùng lặp
     for (uint32_t i = 0; i < uid_count; i++) {
        if (uid_storage[i].length == uid->length &&
            memcmp(uid_storage[i].uid, uid->uid, uid->length) == 0) {
            ESP_LOGW(TAG, "UID already exists");
            xSemaphoreGive(storage_mutex);
            return false;
        }
    }
    uid_storage[uid_count] = *uid;
    uid_count++;
    ESP_LOGI(TAG, "New UID added. Total: %ld", uid_count);
 
    xSemaphoreGive(storage_mutex);
    return true;
}

//ok
bool storage_findUID(RFID_UID_t* uid) {
    if (uid == NULL) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
    bool found = false;
    for (uint32_t i = 0; i < uid_count; i++) {
        if (uid_storage[i].length == uid->length &&
            memcmp(uid_storage[i].uid, uid->uid, uid->length) == 0) {
            found = true;
            break;
        }
    }
    xSemaphoreGive(storage_mutex);
    return found;
}

//ok
bool storage_removeUID(RFID_UID_t* uid) {
    if (uid == NULL || uid_count == 0) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
    int index_to_remove = -1;
    for (uint32_t i = 0; i < uid_count; i++) {
        if (uid_storage[i].length == uid->length &&
            memcmp(uid_storage[i].uid, uid->uid, uid->length) == 0) {
            index_to_remove = (int)i;
            break;
        }
    }
    if (index_to_remove == -1) {
        ESP_LOGW(TAG, "UID not found for removal");
        xSemaphoreGive(storage_mutex);
        return false;
    }
    // Dồn mảng lấp chỗ trống
    for (uint32_t j = (uint32_t)index_to_remove; j < uid_count - 1; j++) {
        uid_storage[j] = uid_storage[j + 1];
    }
    uid_count--;
    memset(&uid_storage[uid_count], 0, sizeof(RFID_UID_t));
    ESP_LOGI(TAG, "UID removed. Remaining: %ld", uid_count);
 
    xSemaphoreGive(storage_mutex);
    return true;
}

//ok
bool storage_getUIDs(char* out_list, size_t buffer_size) {
    if (out_list == NULL || buffer_size == 0) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
        
        out_list[0] = '\0'; // Khởi tạo chuỗi rỗng
       for (uint32_t i = 0; i < uid_count; i++) {
        char uid_hex[RFID_MAX_LENGTH * 2 + 1] = {0};
 
        for (uint8_t j = 0; j < uid_storage[i].length; j++) {
            sprintf(uid_hex + j * 2, "%02X", uid_storage[i].uid[j]);
        }
 
        // Kiểm tra còn đủ chỗ không: uid_hex + dấu cách (hoặc '\0')
        if (strlen(out_list) + strlen(uid_hex) + 2 >= buffer_size) {
            ESP_LOGW(TAG, "Buffer quá nhỏ, danh sách UID bị cắt bớt");
            break;
        }
 
        strcat(out_list, uid_hex);
        if (i < uid_count - 1) {
            strcat(out_list, " ");
        }
    }
    xSemaphoreGive(storage_mutex);
    return true;
}

//mã nguồn admin password
//ok
bool storage_saveAdminPassword(char* password) {
    if (password == NULL) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
    strlcpy(admin_password, password, MAX_ADMIN_PASSWORD_LENGTH);
    ESP_LOGI(TAG, "Admin password saved.");
    xSemaphoreGive(storage_mutex);
    return true;
}

//ok
bool storage_getAdminPassword(void) {
    if (out_password == NULL || max_len == 0) return false;
    if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
    strlcpy(out_password, admin_password, max_len);
    xSemaphoreGive(storage_mutex);
    return true;
}

//giải thích 
//  if (xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) return false;
//  xSemaphoreTake(storage_mutex, portMAX_DELAY): Lệnh kiểm tra xem mutex có free không, 
//  free thì trả về pdTRUE, không thì chờ đợi khi được giải phóng.
//  thứ tự trong hàm trên là tên mutex và thời gian chờ, 
//  portMAX_DELAY có nghĩa là chờ vô thời hạn đến khi nhận được mutex.


