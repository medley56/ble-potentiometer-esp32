/* This contains code related to retrieving potentiometer values written by the ULP process from the ADC 
and publishing them to a queue for later consumption.

The program that runs on the ULP FSM is defined in ulp/adc.S and configured by ulp/ulp_config.h 
*/
#ifndef PUBLISHER_H
#define PUBLISHER_H

/* Define parameters related to polling the value read from the ADC */
#define ADC_CHANGE_TOL           3  // ADC value change that triggers update
#define ADC_CHANGE_POLL_PERIOD  20  // in ms

/* Function that initializes and starts the process of pushing ADC values to a data queue */
void potentiometer_data_service_init(void *pQueue);

#endif