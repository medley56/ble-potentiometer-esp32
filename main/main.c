/* Application for distributing potentiometer values through BLE 

Data is read from the ADC using the ULP FSM coprocessor
The read values are pushed into a queue on Core 1
Values are dequeued and published via BLE on Core 0
*/

/* Standard headers */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* ESP-IDF headers */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

/* ULP-related headers */
#include "ulp_main.h"  // interface to ULP assembly file
#include "publisher.h"

#define QUEUE_WAIT_TICKS        pdMS_TO_TICKS(200)  // Time to wait when retrieving from queue
#define MAIN_LOG_NAME "MAIN"

/* Task that dequeues values and advertises them with a BLE Eddystone beacon */
void ble_beacon_advertiser(void *pvParameters);

void app_main(void)
{
    /* If user is using USB-serial-jtag then idf monitor needs some time to
    *  re-connect to the USB port. We wait 1 sec here to allow for it to make the reconnection
    *  before we print anything. Otherwise the chip will go back to sleep again before the user
    *  has time to monitor any output.
    */
    vTaskDelay(pdMS_TO_TICKS(1000));
    init_ulp_program();

    ESP_LOGI(MAIN_LOG_NAME, "Starting main application\n");
    start_ulp_program();
    ESP_LOGI(MAIN_LOG_NAME, "ULP program started\n");

    QueueHandle_t ulp_value_queue = xQueueCreate(10, sizeof(uint32_t));
    if (ulp_value_queue == NULL) {
        ESP_LOGE(MAIN_LOG_NAME, "Failed to create queue");
    }

    xTaskCreatePinnedToCore(
        ulp_value_publisher,
        "ulp_value_publisher_task",
        2048,
        (void *)ulp_value_queue,  // Pass queue as parameter to task
        5,
        NULL,
        1  // Pin to Core 1. Core 0 is doing other stuff.
    );

    xTaskCreatePinnedToCore(
        ble_beacon_advertiser,
        "ble_beacon_advertiser_task",
        2048,
        (void *)ulp_value_queue,
        1,
        NULL,
        0
    );
}


void ble_beacon_advertiser(void *pvParameters)
{
    QueueHandle_t value_queue = (QueueHandle_t)pvParameters;
    uint32_t advertised_value;

    while (true)
    {
        BaseType_t result = xQueueReceive(value_queue, &advertised_value, QUEUE_WAIT_TICKS);
        if (result == pdPASS) {
            // TODO: Once BLE is working, this should update the value advertised or should send a notification
            ESP_LOGI("BLE", "Value received: %"PRIu32"\n", advertised_value & UINT16_MAX);
        }
    }
}
