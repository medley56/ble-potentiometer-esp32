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

#define ADC_CHANGE_TOL           3  // ADC value change that triggers update
#define ADC_CHANGE_POLL_PERIOD  20  // in ms

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

    uint32_t ulp_previous_result = ulp_last_result;
    uint32_t adc_diff;
    while (true) {
        // Delay so we're not constantly spinning
        vTaskDelay(pdMS_TO_TICKS(ADC_CHANGE_POLL_PERIOD));
        // Calculate if the ADC changed more than the specified tolerance
        adc_diff = (ulp_previous_result > ulp_last_result) ? (ulp_previous_result - ulp_last_result) : (ulp_last_result - ulp_previous_result);
        if (adc_diff > ADC_CHANGE_TOL) {
            printf("ADC value changed! Latest value reported by ULP program is %"PRIu32"\n", 
                   ulp_last_result & UINT16_MAX);
            ulp_previous_result = ulp_last_result;
        }
    }

}
