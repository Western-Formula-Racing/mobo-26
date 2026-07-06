idf_component_register( SRCS "SOC.c"
                        INCLUDE_DIRS "." "../../include"
                        REQUIRES driver BMS io statemachine nvs_flash esp_timer
)
