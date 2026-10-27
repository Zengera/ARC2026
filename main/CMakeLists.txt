idf_component_register(
    SRCS
        "main.cpp"
        "RobotController.cpp"
        "MotorController.cpp"
        "PS4Input.cpp"
        "IRReceiver.cpp"
        "IRremote.cpp"
        "PS4-esp32-develop/src/ps4.c"
        "PS4-esp32-develop/src/ps4_l2cap.c"
        "PS4-esp32-develop/src/ps4_parser.c"
        "PS4-esp32-develop/src/ps4_spp.c"
    INCLUDE_DIRS
        "."
        "PS4-esp32-develop/src"
    PRIV_REQUIRES
        bt
        nvs_flash
        esp_timer
        esp_hw_support
        esp_driver_ledc
        esp_driver_gpio
        esp_driver_rmt
)



