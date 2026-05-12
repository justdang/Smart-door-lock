#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "auth.h"
#include "rfid_struct.h"
#include "storage.h"
#define MAX_PASSWORD_LENGTH 6
#define MAX_ADMIN_PASSWORD_LENGTH 6
#define MAX_UIDS 10

//password
static char user_password[MAX_PASSWORD_LENGTH] = "123456"; //mật khẩu cho user (mặc định)
static char admin_password[MAX_ADMIN_PASSWORD_LENGTH] = "111111"; //mật khẩu cho admin
//UID
static RFID_UID_t uid_storage[MAX_UIDS];
static uint8_t uid_count = 0;

void storage_init(void){
    memset(user_password, 0, sizeof(user_password));
    memset(admin_password, 0, sizeof(admin_password));
    memset(uid_storage, 0, sizeof(uid_storage));
    uid_count = 0;
}

//mã nguồn password
bool storage_savePassword(char* password) {
    strncpy(user_password, password, MAX_PASSWORD_LENGTH);
    return true;
} // sửa lại

char *storage_getPassword(void) {
    return user_password;
}

//rfid 
bool storage_addUID(RFID_UID_t* uid) {
    if (uid_count < MAX_UIDS) {
        uid_storage[uid_count++] = *uid;
        return true;
    }
    return false; // bộ nhớ đầy
}
bool storage_findUID(RFID_UID_t* uid) {
    for (uint8_t i = 0; i < uid_count; i++) {
        if (memcmp(uid_storage[i].uid, uid->uid, uid->length) == 0) {
            return true; // tìm thấy UID
        }
    }
    return false; // không tìm thấy UID
}
bool storage_removeUID(RFID_UID_t* uid){
    for(uint8_t i = 0; i < uid_count; i++){
        if(storage_findUID(uid) == true){
            // Shift các phần tử còn lại lên trước
            for (uint8_t j = i; j < uid_count - 1; j++) {
                uid_storage[j] = uid_storage[j + 1];
            }
            memset(&uid_storage[--uid_count], 0, sizeof(RFID_UID_t)); // Xóa phần tử cuối cùng
            return true;
        }
        else return false;
    }
}
char* storage_getUIDs(void){
    static char uid_list[MAX_UIDS * (RFID_MAX_LENGTH * 2 + 3)]; // Mỗi UID có thể chiếm tối đa 2 ký tự hex + dấu cách
    uid_list[0] = '\0'; // Khởi tạo chuỗi rỗng

    for (uint8_t i = 0; i < uid_count; i++) {
        char uid_str[RFID_MAX_LENGTH * 2 + 1]; // Mỗi byte có thể chiếm tối đa 2 ký tự hex
        for (uint8_t j = 0; j < uid_storage[i].length; j++) {
            sprintf(uid_str + j * 2, "%02X", uid_storage[i].uid[j]);
        }
        strcat(uid_list, uid_str);
        if (i < uid_count - 1) {
            strcat(uid_list, " "); // Thêm dấu cách giữa các UID
        }
    }
    return uid_list;
}

//mã nguồn admin password
bool storage_saveAdminPassword(char* password) {
    strncpy(admin_password, password, MAX_ADMIN_PASSWORD_LENGTH);
    return true;
}
char *storage_getAdminPassword(void) {
    return admin_password;
}


