#include "consumer.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ble.h"

static uint16_t potentiometer_value;

void update_potentiometer_value(void *pQueue)
{
    QueueHandle_t queue = (QueueHandle_t)pQueue;
    while (true)
    {
        BaseType_t result = xQueueReceive(queue, &potentiometer_value, DEQUEUE_WAIT_MS);
        if (result == pdPASS) {
            ESP_LOGI("CONSUMER", "New potentiometer value sent to BLE: %"PRIu16"\n", potentiometer_value);
        }
        vTaskDelay(pdMS_TO_TICKS(DEQUEUE_WAIT_MS));
    }
}

uint16_t get_potentiometer_value(void)
{
    return potentiometer_value;
}


void potentiometer_data_consumer_init(void *pQueue)
{
    xTaskCreatePinnedToCore(
        update_potentiometer_value,
        "Consumer",
        2048,
        pQueue,
        CONSUMER_PRIORITY,
        NULL,
        CONSUMER_CORE
    );
}
