#pragma once

#include <stdint.h>

void initTelemetry();
void telemetryQueueFrame(uint32_t canId, uint8_t *data, uint8_t len);
