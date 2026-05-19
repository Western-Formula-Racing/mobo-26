idf_component_register(SRCS "state-of-charge.c"
                       INCLUDE_DIRS "." "../../include" 
                       REQUIRES io CAN nvs_flash esp_timer)