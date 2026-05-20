#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <string.h>
#include "nvs_flash.h" // có sẵn của espidf
#include "nvs.h" // có sẵn của espidf
#include "esp_log.h" // có sẵn của espidf
#include "rfid_struct.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // giai thich thu vien nay 
#define MAX_PASSWORD_LENGTH 7 //6 ký tự và ký tự "\0"
#define MAX_ADMIN_PASSWORD_LENGTH 7
#define MAX_UIDS 10

//password
static char user_password[MAX_PASSWORD_LENGTH] = "123456"; //mật khẩu cho user (mặc định)
static char admin_password[MAX_ADMIN_PASSWORD_LENGTH] = "111111"; //mật khẩu cho admin
//UID
static RFID_UID_t uid_storage[MAX_UIDS];
static uint32_t uid_count = 0;
//mutex
static SemaphoreHandle_t storage_mutex = NULL;

//hàm nội bộ
static void _storage_load_logic(void) {
    // memset(user_password, 0, sizeof(user_password));
    // memset(admin_password, 0, sizeof(admin_password));
    // memset(uid_storage, 0, sizeof(uid_storage));
    // uid_count = 0;
    //đọc nvs
    nvs_handle_t h;
    if (nvs_open("storage", NVS_READONLY, &h) == ESP_OK) {
        size_t sz = sizeof(user_password);
        nvs_get_str(h, "u_pass", user_password, &sz);
        sz = sizeof(admin_password);
        nvs_get_str(h, "a_pass", admin_password, &sz);
        nvs_get_u32(h, "u_count", &uid_count);
        sz = sizeof(uid_storage);
        nvs_get_blob(h, "u_ids", uid_storage, &sz);
        nvs_close(h);
        ESP_LOGI(TAG, "Data loaded from NVS.");
    }
}
static void _storage_save_to_nvs(void) {
    nvs_handle_t h;
    if (nvs_open("storage", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "u_pass", user_password);
        nvs_set_str(h, "a_pass", admin_password);
        nvs_set_u32(h, "u_count", uid_count);
        nvs_set_blob(h, "u_ids", uid_storage, sizeof(uid_storage));
        nvs_commit(h);
        nvs_close(h);
    }
}
void storage_init(void){
    if(storage_mutex == NULL) storage_mutex = xSemaphoreCreateMutex();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }
    //giải thích
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        // Nếu chưa chạy đa nhiệm, gọi thẳng hàm logic
        _storage_load_logic();
    } else {
        // Nếu đã chạy đa nhiệm, bắt buộc dùng Mutex
        if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            _storage_load_logic();
            xSemaphoreGive(storage_mutex);
        } 
        // else {
        //     ESP_LOGE(TAG, "Không thể lấy Mutex (Deadlock potential hoặc Task khác giữ quá lâu)!");
        // }
    }
}
//mã nguồn password
bool storage_savePassword(char* password) {
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Sử dụng strlcpy thay vì strncpy để luôn có '\0' ở cuối
        strlcpy(user_password, password, MAX_PASSWORD_LENGTH);
        _storage_save_to_nvs(); // Đã sửa: Lưu vào NVS luôn khi đổi pass
        xSemaphoreGive(storage_mutex);
        return true;
    }
    return false;
}
//get password
bool storage_getPassword(char* out_password, size_t max_len) {
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        strlcpy(out_password, user_password, max_len);
        xSemaphoreGive(storage_mutex);
        return true;
    }
    return false; 
}

//rfid 
bool storage_addUID(RFID_UID_t* uid) {
    if (uid == NULL || uid_count >= MAX_UIDS) {
        ESP_LOGW(TAG, "Storage full or invalid UID");
        return false;
    }
    // 1. Kiểm tra xem UID này đã tồn tại chưa để tránh trùng lặp
    if (storage_findUID(uid)) {
        ESP_LOGW(TAG, "UID already exists");
        return false;
    }
    bool success = false;
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // 2. Thêm vào mảng trong RAM
        uid_storage[uid_count] = *uid;
        uid_count++;

        // 3. Gọi hàm lưu xuống NVS (hàm này sẽ kích hoạt AES-256 mã hóa dữ liệu)
        _storage_save_to_nvs(); 
        
        xSemaphoreGive(storage_mutex);
        ESP_LOGI(TAG, "New UID added and encrypted to NVS. Total: %ld", uid_count);
        success = true;
    }
    return success;
}
bool storage_findUID(RFID_UID_t* uid) {
    if (uid == NULL) return false;
    bool found = false;
    // Khóa Mutex để đảm bảo mảng không bị thay đổi khi đang duyệt
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint32_t i = 0; i < uid_count; i++) {
            // So sánh chiều dài và nội dung UID
            if (uid_storage[i].length == uid->length) {
                if (memcmp(uid_storage[i].uid, uid->uid, uid->length) == 0) {
                    found = true;
                    break; 
                }
            }
        }
        xSemaphoreGive(storage_mutex);
    }
    return found;
}
bool storage_removeUID(RFID_UID_t* uid) {
    if (uid == NULL || uid_count == 0) return false;
    bool success = false;
    // 1. Khóa Mutex để bảo vệ mảng trong suốt quá trình xóa và dồn mảng
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        int index_to_remove = -1;
        // 2. Tìm vị trí (index) của UID cần xóa
        for (uint32_t i = 0; i < uid_count; i++) {
            if (uid_storage[i].length == uid->length) {
                if (memcmp(uid_storage[i].uid, uid->uid, uid->length) == 0) {
                    index_to_remove = i;
                    break;
                }
            }
        }
        // 3. Nếu tìm thấy, tiến hành xóa và dồn mảng
        if (index_to_remove != -1) {
            // Shift các phần tử phía sau lên trước để lấp chỗ trống
            for (uint32_t j = index_to_remove; j < uid_count - 1; j++) {
                uid_storage[j] = uid_storage[j + 1];
            }
            // Giảm số lượng UID và xóa trắng phần tử thừa cuối cùng trong RAM
            uid_count--;
            memset(&uid_storage[uid_count], 0, sizeof(RFID_UID_t));
            // 4. Đồng bộ hóa xuống NVS (Dữ liệu mới sẽ được mã hóa AES-256)
            _storage_save_to_nvs();
            ESP_LOGI(TAG, "UID removed. Remaining: %ld", uid_count);
            success = true;
        } else {
            ESP_LOGW(TAG, "UID not found for removal");
        }
        xSemaphoreGive(storage_mutex);
    }
    return success;
}
bool storage_getUIDs(char* out_list, size_t buffer_size) {
    if (out_list == NULL || buffer_size == 0) return false;

    bool success = false;
    // 1. Khóa Mutex để đảm bảo mảng uid_storage không bị thay đổi (add/remove) khi đang đọc
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        
        out_list[0] = '\0'; // Khởi tạo chuỗi rỗng
        char temp_str[RFID_MAX_LENGTH * 2 + 2]; // Buffer tạm cho từng UID

        for (uint32_t i = 0; i < uid_count; i++) {
            char uid_hex[RFID_MAX_LENGTH * 2 + 1] = {0};
            
            // Chuyển từng byte UID sang HEX
            for (uint8_t j = 0; j < uid_storage[i].length; j++) {
                sprintf(uid_hex + j * 2, "%02X", uid_storage[i].uid[j]);
            }

            // Kiểm tra xem còn đủ chỗ trong buffer out_list không
            // (Độ dài hiện tại + UID mới + khoảng trắng + ký tự null)
            if (strlen(out_list) + strlen(uid_hex) + 2 < buffer_size) {
                strcat(out_list, uid_hex);
                if (i < uid_count - 1) {
                    strcat(out_list, " ");
                }
            } else {
                ESP_LOGW(TAG, "Buffer cung cấp quá nhỏ để chứa toàn bộ danh sách UID");
                break;
            }
        }
        xSemaphoreGive(storage_mutex);
        success = true;
    }
    return success;
}

//mã nguồn admin password
bool storage_saveAdminPassword(char* password) {
    bool success = false;
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        strlcpy(admin_password, password, MAX_ADMIN_PASSWORD_LENGTH);
        _storage_save_to_nvs(); 
        xSemaphoreGive(storage_mutex);
        ESP_LOGI(TAG, "Admin Password updated and encrypted.");
        success = true;
    }
}
bool storage_getAdminPassword(void) {
    bool success = false;
    // Khóa Mutex để tránh việc Task Admin đang đọc thì Task khác đang ghi đè
    if (xSemaphoreTake(storage_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        strlcpy(out_password, admin_password, max_len);
        xSemaphoreGive(storage_mutex);
        success = true;
    }
    return success;
}


