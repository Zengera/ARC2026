idf_component_register(
    SRCS "main.c" "motor_driver.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash bt driver esp_driver_ledc esp_driver_gpio log PS4Controller
)



