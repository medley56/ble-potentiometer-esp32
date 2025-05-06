#include "potentiometer.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static uint16_t potentiometer_value;

void update_potentiometer_value(void *pQueue)
{
    QueueHandle_t queue = (QueueHandle_t)pQueue;
    BaseType_t result = xQueueReceive(queue, &potentiometer_value, QUEUE_WAIT_TICKS);
    if (result == pdPASS) {
        ESP_LOGI("POTENTIOMETER", "New potentiometer value: %"PRIu16"\n", potentiometer_value);
    }
}

uint16_t get_potentiometer_value(void)
{
    return potentiometer_value;
}
