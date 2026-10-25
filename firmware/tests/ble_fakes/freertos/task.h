#pragma once
#include "FreeRTOS.h"
void vTaskDelete(void *);
int xTaskCreate(void (*)(void *), const char *, unsigned, void *, unsigned, void *);
void vTaskDelay(TickType_t);
