/* This contains code related to retrieving potentiometer values written by the ULP process from the ADC 
and publishing them to a queue for later consumption.

The program that runs on the ULP FSM is defined in ulp/adc.S and configured by ulp/ulp_config.h 
*/
#ifndef PRODUCER_H
#define PRODUCER_H

/* Define parameters related to polling the value read from the ADC */
#define ADC_CHANGE_TOL          10  // ADC value change that triggers update
#define ADC_CHANGE_POLL_PERIOD  200  // in ms (5Hz)
#define ULP_WAKEUP_PERIOD_US  500000  // 50 milliseconds (20Hz)
#define PRODUCER_CORE  1
#define PRODUCER_PRIORITY 5

/* Function that initializes and starts the process of pushing ADC values to a data queue */
void potentiometer_data_producer_init(void *pQueue);

#endif