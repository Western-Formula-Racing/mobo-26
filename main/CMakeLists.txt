idf_component_register(SRCS "main.c" INCLUDE_DIRS "." "../components" "../include" REQUIRES "io" "CAN" "periodic" "BMS" "statemachine" "emeter" "ds18x20" "onewire" "coloumb-counting")
