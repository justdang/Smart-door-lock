idf_component_register(
    SRCS "storage.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS ".."
    # REQUIRES nvs_flash
    # REQUIRES nvs
    REQUIRES common
)