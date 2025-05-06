/* Implementations for publisher.h */

/* Header */
#include "publisher.h"

/* ESP-IDF headers */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "ulp.h"
#include "ulp_adc.h"
#include "driver/rtc_io.h"
#include "ulp_common_defs.h"

/* ULP config and ASM-generated header */
#include "ulp_main.h"  // Generated from adc.S via configs in CMakeLists.txt
#include "ulp/ulp_config.h"  // Configurations for adc.S as a readable header

#define PUBLISHER_LOG_NAME "PUBLISHER"

/* Location of ULP binary in the codespace */
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

/* Value stored by ULP program (read from ADC) on each ULP execution */
extern uint32_t ulp_last_result;


/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC.
 */
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

    /* Disconnect GPIO12 and GPIO15 to remove current drain through
     * pullup/pulldown resistors on modules which have these (e.g. ESP32-WROVER)
     * GPIO12 may be pulled high to select flash voltage.
     */
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_15);

    esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}


/* This function is called once during initialization. It starts the ULP FSM running. */
static void start_ulp_program(void)
{
    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}


/* Task that checks for new values from the ULP and publishes them to a queue */
static void ulp_value_publisher_task(void *pvParameters)
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
            ESP_LOGI(PUBLISHER_LOG_NAME, "ADC value changed! Latest value reported by ULP program is %"PRIu32"\n", 
                ulp_last_result & UINT16_MAX);
            // Send new value to the queue for later consumption
            BaseType_t result = xQueueSendToBack(value_queue, &ulp_last_result, 0);
            if (result != pdPASS) {
                ESP_LOGE(PUBLISHER_LOG_NAME, "Failed to push value to queue. This may mean that the subscriber is not consuming data fast enough.\n");
            }
            ulp_previous_result = ulp_last_result;
        }
    }
}


void potentiometer_data_service_init(void *pQueue)
{
    QueueHandle_t queue = (QueueHandle_t)pQueue;
    /* Initialize the ULP and start sampling the ADC */
    init_ulp_program();
    start_ulp_program();
    ESP_LOGI(PUBLISHER_LOG_NAME, "ULP ADC-sampling program started\n");

    /* Start the task that pushes ADC values sampled by the ULP to a data queue */
    xTaskCreatePinnedToCore(
        ulp_value_publisher_task,
        "Publisher Task",
        2048,
        (void *)queue,  // Pass queue as parameter to task
        5,
        NULL,
        1  // Pin to Core 1. Core 0 is doing other stuff.
    );
}
