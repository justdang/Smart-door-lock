# cmake_minimum_required(VERSION 3.16.0)
# include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# project(smart_door_lock)

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "components/service/auth"
    "components/service/common"
    "components/service/display_service"
    "components/service/keypad_service"
    "components/service/rfid_service"
    "components/service/rtc_service"
    "components/service/servo_service"
    "components/service/storage"
    "components/service/zigbee"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Smart-door-lock)