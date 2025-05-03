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
#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "ulp.h"
#include "ulp_main.h"
#include "esp_adc/adc_oneshot.h"
#include "ulp/ulp_config.h"
#include "ulp_adc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");
extern uint32_t ulp_tolerance;
extern uint32_t ulp_last_result;

#define ADC_CHANGE_TOL   5  // ADC value change that triggers update
#define ADC_CHANGE_POLL_PERIOD  20  // in ticks 

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC.
 */
static void init_ulp_program(void);

/* This function is called during initialization.
 * It starts the ULP program and resets measurement counter.
 */
static void start_ulp_program(void);

void app_main(void)
{
    /* If user is using USB-serial-jtag then idf monitor needs some time to
    *  re-connect to the USB port. We wait 1 sec here to allow for it to make the reconnection
    *  before we print anything. Otherwise the chip will go back to sleep again before the user
    *  has time to monitor any output.
    */
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not ULP wakeup\n");
        init_ulp_program();
    }

    printf("Starting main application\n");
    start_ulp_program();
    printf("ULP program started\n");

    uint32_t ulp_previous_result = ulp_last_result;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(ADC_CHANGE_POLL_PERIOD));
        if (abs(ulp_last_result - ulp_previous_result) > ADC_CHANGE_TOL) {
            printf("ADC value changed! Latest value reported by ULP program is %"PRIu32"\n", 
                   ulp_last_result & UINT16_MAX);
            ulp_previous_result = ulp_last_result;
        }
    }

}

static void init_ulp_program(void)
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
            (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    ulp_adc_cfg_t cfg = {
        .adc_n    = ULP_ADC_UNIT,
        .channel  = ULP_ADC_CHANNEL,
        .width    = ULP_ADC_BITWIDTH,
        .atten    = ULP_ADC_ATTEN,
        .ulp_mode = ADC_ULP_MODE_FSM,
    };

    ESP_ERROR_CHECK(ulp_adc_init(&cfg));

    /* Set ULP wake up period to 200ms (5Hz).
     * This sets the SENS_ULP_CP_SLEEP_CYC0_REG.
     * There are 5 of these registers available (CYC0..CYC4) but 0 is used 
     * by default on ESP32 boards. 
     */
    ulp_set_wakeup_period(0, 200000);

#if CONFIG_IDF_TARGET_ESP32
    /* Disconnect GPIO12 and GPIO15 to remove current drain through
     * pullup/pulldown resistors on modules which have these (e.g. ESP32-WROVER)
     * GPIO12 may be pulled high to select flash voltage.
     */
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_15);
#endif // CONFIG_IDF_TARGET_ESP32

    esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}

static void start_ulp_program(void)
{
    /* Reset sample counter */
    ulp_sample_counter = 0;

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
