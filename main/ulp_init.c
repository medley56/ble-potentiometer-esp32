/* Implementations for ulp_init.h */

#include "ulp_init.h"

#include "esp_err.h"
#include "esp_sleep.h"
#include "ulp.h"
#include "ulp_adc.h"
#include "driver/rtc_io.h"
#include "ulp_common_defs.h"

#include "ulp_main.h"
#include "ulp/ulp_config.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");
extern uint32_t ulp_last_result;


void init_ulp_program(void)
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

void start_ulp_program(void)
{
    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
