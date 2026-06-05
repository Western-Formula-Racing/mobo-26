idf_component_register(SRCS "coloumb-counting.c"
                       INCLUDE_DIRS "." "../../include" 
                       REQUIRES io CAN nvs_flash esp_timer)