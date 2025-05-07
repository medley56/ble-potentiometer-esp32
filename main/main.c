/* Application for distributing potentiometer values through BLE 

Data is read from the ADC using the ULP FSM coprocessor
The read values are published into a queue on Core 1
Values are dequeued by a subscriber task and sent to BLE on Core 0
*/

/* Standard headers */
#include <inttypes.h>

/* ESP-IDF headers */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

/* Application module headers */
#include "ulp_main.h"  // interface to ULP assembly file
#include "producer.h"  // publishing service for potentiometer values
#include "consumer.h"
#include "ble.h"  // BLE services, including task that subscribes to the ADC data queue and updates the BLE value

#define MAIN_LOG_NAME "MAIN"


void app_main(void)
{
    /* If user is using USB-serial-jtag then idf monitor needs some time to
    *  re-connect to the USB port. We wait 1 sec here to allow for it to make the reconnection
    *  before we print anything. Otherwise the chip will go back to sleep again before the user
    *  has time to monitor any output.
    */
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(MAIN_LOG_NAME, "Starting main application\n");

    QueueHandle_t ulp_value_queue = xQueueCreate(10, sizeof(uint32_t));
    if (ulp_value_queue == NULL) {
        ESP_LOGE(MAIN_LOG_NAME, "Failed to create queue");
    }

    /* Start the ULP program and the task that puts values into the queue */
    potentiometer_data_producer_init(ulp_value_queue);

    /* Start the task that dequeues values and passes them to BLE */
    potentiometer_data_consumer_init(ulp_value_queue);

    /* Start the BLE stack that advertises, connects, and notifies of new values 
    via a GATT service characteristic */
    ble_init();
}
