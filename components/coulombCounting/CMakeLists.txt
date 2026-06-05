idf_component_register(SRCS "coulombCounting.c"
                       INCLUDE_DIRS "." "../../include" 
                       REQUIRES io CAN nvs_flash esp_timer)