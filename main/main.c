/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ULP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ulp_main.h"  // interface to ULP assembly file
#include "ulp_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define ADC_CHANGE_TOL           3  // ADC value change that triggers update
#define ADC_CHANGE_POLL_PERIOD  20  // in ms
#define QUEUE_WAIT_TICKS        pdMS_TO_TICKS(200)  // Time to wait when retrieving from queue

/* Task that checks for new values from the ULP and publishes them to a queue */
void ulp_value_publisher(void *pvParameters);

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

    printf("Starting main application\n");
    start_ulp_program();
    printf("ULP program started\n");

    QueueHandle_t ulp_value_queue = xQueueCreate(10, sizeof(uint32_t));
    if (ulp_value_queue == NULL) {
        ESP_LOGE("ULP", "Failed to create queue");
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


void ulp_value_publisher(void *pvParameters)
{
    QueueHandle_t value_queue = (QueueHandle_t)pvParameters;
    TickType_t poll_period_ticks = pdMS_TO_TICKS(ADC_CHANGE_POLL_PERIOD);
    uint32_t ulp_previous_result = ulp_last_result;
    uint32_t adc_diff;
    while (true)
    {
        // Delay so we're not constantly spinning
        vTaskDelay(poll_period_ticks);
        // Calculate if the ADC changed more than the specified tolerance
        adc_diff = (ulp_previous_result > ulp_last_result) ? (ulp_previous_result - ulp_last_result) : (ulp_last_result - ulp_previous_result);
        if (adc_diff > ADC_CHANGE_TOL) {
            printf("ADC value changed! Latest value reported by ULP program is %"PRIu32"\n", 
                   ulp_last_result & UINT16_MAX);
            BaseType_t result = xQueueSendToBack(value_queue, &ulp_last_result, 0);
            if (result != pdPASS) {
                printf("Failed to push value to queue. This may mean that the subscriber is not consuming data fast enough.\n");
            }
            ulp_previous_result = ulp_last_result;
        }
    }
}


void ble_beacon_advertiser(void *pvParameters)
{
    QueueHandle_t value_queue = (QueueHandle_t)pvParameters;
    uint32_t advertised_value;

    while (true)
    {
        BaseType_t result = xQueueReceive(value_queue, &advertised_value, QUEUE_WAIT_TICKS);
        if (result != pdPASS) {
            printf("No value received from queue within wait time\n");
        } else {
            printf("Value received: %"PRIu32"\n", advertised_value & UINT16_MAX);
        }
    }
}
