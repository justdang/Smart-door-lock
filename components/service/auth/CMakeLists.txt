idf_component_register(
    SRCS
        "auth_service.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        storage
        common
        keypad_service
        rfid_service
        zigbee 
        freertos
)