/project
│
├── app/
│   ├── app_main.c
│   ├── app_fsm.c          
│   └── app_fsm.h  //state machine
│
├── service/
│   ├── auth/
│   │   ├── auth.c
│   │   └── auth.h
│   │
│   ├── keypad_service/
│   │   ├── keypad_service.c
│   │   └── keypad_service.h
│   │
│   ├── rfid_service/
│   │   ├── rfid_service.c
│   │   └── rfid_service.h
│   │
│   ├── storage/
│   │   ├── storage.c
│   │   └── storage.h
│   │
│   ├── servo_service/
│   │   ├── servo_service.c
│   │   └── servo_service.h
│   │
│   ├── display_service/
│   │   ├── dispkay_service.c
│   │   └── display_service.h
│   │
│   ├── access_control/
│   │   ├── access_control.c
│   │   └── access_control.h
│   │
│   └── rtc_service/
│       ├── rtc_service.c
│       └── rtc_service.h   
│   
├── driver/
│   ├── gpio/ 
│   ├── uart/
│   ├── timer/
│   ├── keypad_driver/
│   │   ├── keypad_driver.c
│   │   └── keypad_driver.h
│   │
│   ├── rfid_driver/
│   │   ├── rfid_driver.c
│   │   └── rfid_driver.h
│   │
│   ├── rtc_driver/
│   │   ├── rtc_driver.c
│   │   └── rtc_driver.h
│   │
│   ├── servo_driver/
│   │   ├── servo_driver.c
│   │   └── servo_driver.h
│   │
│   └── display_driver/
│       ├── display_driver.c
│       └── display_driver.h
│
├── bsp/
│   └── bsp_init.c         // init hệ thống và ánh xạ phần cứng với driver
│
├── config/
│   └── system_config.h    // chứa các tham số để khi thay đổi tham số thì không cần thay đổi logic 
│
└── main.c