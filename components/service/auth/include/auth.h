#include "auth_service.h"
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_partition.h" // Thư viện quản lý phân vùng để tìm key AES

static const char *TAG = "AUTH_SERVICE_AES";

#define NVS_AUTH_NAMESPACE "auth_store"
#define KEY_USER_PASS      "user_pass"
#define KEY_ADMIN_PASS     "admin_pass"
#define KEY_RFID_COUNT     "rfid_count"
#define KEY_RFID_LIST      "rfid_list"

#define MAX_RFID_CARDS     10
#define MAX_PASS_LEN       32
#define DEFAULT_PASSWORD   "123456"

// Khởi tạo Auth Service bảo mật bằng AES-128 thông qua NVS Encryption
void auth_init(void) {
    // 1. Tìm phân vùng chứa khóa mã hóa mã tên "nvs_keys" trong Partition Table
    const esp_partition_t* key_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, 
        ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, 
        "nvs_keys"
    );

    if (key_part == NULL) {
        ESP_LOGE(TAG, "Không tìm thấy phân vùng 'nvs_keys'. Hãy kiểm tra file partitions.csv!");
        return;
    }

    // 2. Đọc cấu hình bảo mật (chứa khóa mã hóa AES) từ phân vùng key
    nvs_sec_cfg_t cfg;
    esp_err_t err = nvs_flash_read_security_cfg(key_part, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Lỗi đọc cấu hình bảo mật mã hóa NVS (%s)", esp_err_to_name(err));
        return;
    }

    // 3. Khởi tạo phân vùng NVS bằng chế độ bảo mật mã hóa AES-128
    err = nvs_flash_secure_init(&cfg);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_secure_init(&cfg);
    }
    ESP_ERROR_CHECK(err);

    // 4. Mở namespace để thiết lập dữ liệu mặc định ban đầu nếu trống
    nvs_handle_t handle;
    err = nvs_open(NVS_AUTH_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    char dummy[MAX_PASS_LEN];
    size_t req_len = MAX_PASS_LEN;
    
    // Kiểm tra và khởi tạo mật khẩu User mặc định dạng thuần túy 
    // (Driver NVS sẽ tự động mã hóa AES khi ghi xuống Flash)
    if (nvs_get_str(handle, KEY_USER_PASS, dummy, &req_len) == ESP_ERR_NVS_NOT_FOUND) {
        nvs_set_str(handle, KEY_USER_PASS, DEFAULT_PASSWORD);
        nvs_commit(handle);
        ESP_LOGI(TAG, "Đã khởi tạo mật khẩu User mặc định qua AES-128");
    }

    // Khởi tạo mật khẩu Admin mặc định
    req_len = MAX_PASS_LEN;
    if (nvs_get_str(handle, KEY_ADMIN_PASS, dummy, &req_len) == ESP_ERR_NVS_NOT_FOUND) {
        nvs_set_str(handle, KEY_ADMIN_PASS, DEFAULT_PASSWORD);
        nvs_commit(handle);
        ESP_LOGI(TAG, "Đã khởi tạo mật khẩu Admin mặc định qua AES-128");
    }

    // Kiểm tra biến đếm số lượng thẻ RFID
    uint32_t rfid_count = 0;
    if (nvs_get_u32(handle, KEY_RFID_COUNT, &rfid_count) == ESP_ERR_NVS_NOT_FOUND) {
        nvs_set_u32(handle, KEY_RFID_COUNT, 0);
        nvs_commit(handle);
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Hệ thống bảo mật mã hóa AES hoàn tất khởi tạo.");
}

// --- PASSWORD MANAGEMENT ---

bool auth_checkPassword(char* password) {
    if (password == NULL) return false;
    
    char stored_pass[MAX_PASS_LEN];
    size_t len = MAX_PASS_LEN;

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;
    
    // NVS tự động giải mã dữ liệu AES từ Flash thành text thô nạp vào stored_pass
    esp_err_t err = nvs_get_str(handle, KEY_USER_PASS, stored_pass, &len);
    nvs_close(handle);

    if (err == ESP_OK && strcmp(password, stored_pass) == 0) {
        return true;
    }
    return false;
}

bool auth_setPassword(char* old_pass, char* new_pass, char* confirm_pass) {
    if (old_pass == NULL || new_pass == NULL || confirm_pass == NULL) return false;
    if (strcmp(new_pass, confirm_pass) != 0) return false;
    if (!auth_checkPassword(old_pass)) return false;

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;

    // Ghi chuỗi text mới, NVS tự dùng khóa AES-128 để xáo trộn dữ liệu trước khi nạp vào Flash
    esp_err_t err = nvs_set_str(handle, KEY_USER_PASS, new_pass);
    if (err == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return (err == ESP_OK);
}

// --- RFID MANAGEMENT ---

bool auth_checkRFID(RFID_UID_t* uid) {
    if (uid == NULL) return false;

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;

    uint32_t count = 0;
    nvs_get_u32(handle, KEY_RFID_COUNT, &count);
    if (count == 0) {
        nvs_close(handle);
        return false;
    }

    RFID_UID_t list[MAX_RFID_CARDS];
    size_t len = sizeof(list);
    // Danh sách thẻ được giải mã AES tự động đổ vào mảng struct
    esp_err_t err = nvs_get_blob(handle, KEY_RFID_LIST, list, &len);
    nvs_close(handle);

    if (err != ESP_OK) return false;

    for (int i = 0; i < count; i++) {
        if (list[i].size == uid->size && memcmp(list[i].uidByte, uid->uidByte, uid->size) == 0) {
            return true; 
        }
    }
    return false;
}

bool auth_setRFID(RFID_UID_t* uid) {
    if (uid == NULL) return false;
    if (auth_checkRFID(uid)) return false; 

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;

    uint32_t count = 0;
    nvs_get_u32(handle, KEY_RFID_COUNT, &count);
    if (count >= MAX_RFID_CARDS) {
        nvs_close(handle);
        return false;
    }

    RFID_UID_t list[MAX_RFID_CARDS];
    size_t len = sizeof(list);
    if (count > 0) {
        nvs_get_blob(handle, KEY_RFID_LIST, list, &len);
    }

    list[count] = *uid;
    count++;

    // Toàn bộ mảng struct RFID sẽ được mã hóa AES-128 khi lưu xuống phần cứng
    nvs_set_blob(handle, KEY_RFID_LIST, list, count * sizeof(RFID_UID_t));
    nvs_set_u32(handle, KEY_RFID_COUNT, count);
    
    esp_err_t err = nvs_commit(handle);
    nvs_close(handle);
    return (err == ESP_OK);
}

bool auth_removeRFID(RFID_UID_t* uid) {
    if (uid == NULL) return false;

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;

    uint32_t count = 0;
    nvs_get_u32(handle, KEY_RFID_COUNT, &count);
    if (count == 0) {
        nvs_close(handle);
        return false;
    }

    RFID_UID_t list[MAX_RFID_CARDS];
    size_t len = sizeof(list);
    if (nvs_get_blob(handle, KEY_RFID_LIST, list, &len) != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    int index_to_remove = -1;
    for (int i = 0; i < count; i++) {
        if (list[i].size == uid->size && memcmp(list[i].uidByte, uid->uidByte, uid->size) == 0) {
            index_to_remove = i;
            break;
        }
    }

    if (index_to_remove == -1) {
        nvs_close(handle);
        return false;
    }

    for (int i = index_to_remove; i < count - 1; i++) {
        list[i] = list[i + 1];
    }
    count--;

    nvs_set_u32(handle, KEY_RFID_COUNT, count);
    if (count > 0) {
        nvs_set_blob(handle, KEY_RFID_LIST, list, count * sizeof(RFID_UID_t));
    } else {
        nvs_erase_key(handle, KEY_RFID_LIST);
    }

    esp_err_t err = nvs_commit(handle);
    nvs_close(handle);
    return (err == ESP_OK);
}

// --- ADMIN MANAGEMENT ---

bool auth_checkAdminPassword(char* password) {
    if (password == NULL) return false;
    char stored_pass[MAX_PASS_LEN];
    size_t len = MAX_PASS_LEN;

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return false;
    
    esp_err_t err = nvs_get_str(handle, KEY_ADMIN_PASS, stored_pass, &len);
    nvs_close(handle);

    if (err == ESP_OK && strcmp(password, stored_pass) == 0) {
        return true;
    }
    return false;
}

bool auth_setAdminPassword(char* old_admin_pass, char* new_admin_pass, char* confirm_admin_pass) {
    if (old_admin_pass == NULL || new_admin_pass == NULL || confirm_admin_pass == NULL) return false;
    if (strcmp(new_admin_pass, confirm_admin_pass) != 0) return false;
    if (!auth_checkAdminPassword(old_admin_pass)) return false;

    nvs_handle_t handle;
    if (nvs_open(NVS_AUTH_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;

    esp_err_t err = nvs_set_str(handle, KEY_ADMIN_PASS, new_admin_pass);
    if (err == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return (err == ESP_OK);
}