/* This contains code related to retrieving potentiometer values written by the ULP process from the ADC 
and publishing them to a queue for later consumption.

The program that runs on the ULP FSM is defined in ulp/adc.S and configured by ulp/ulp_config.h 
*/

/* Define parameters related to polling the value read from the ADC */
#define ADC_CHANGE_TOL           3  // ADC value change that triggers update
#define ADC_CHANGE_POLL_PERIOD  20  // in ms

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC.
 */
void init_ulp_program(void);

/* This function is called once during initialization. It starts the ULP FSM running. */
void start_ulp_program(void);

/* Task that checks for new values from the ULP and publishes them to a queue */
void ulp_value_publisher(void *pvParameters);