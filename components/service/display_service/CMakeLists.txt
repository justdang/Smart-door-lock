idf_component_register(
    SRCS
        "display_service.c"
    INCLUDE_DIRS
        "include"
     REQUIRES
        display_driver       ← wherever display_driver.h lives
        freertos
        esp_common
)