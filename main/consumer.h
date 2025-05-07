#ifndef CONSUMER_H
#define CONSUMER_H

#include <inttypes.h>

#define DEQUEUE_WAIT_MS 100  // 100ms (10Hz)
#define CONSUMER_CORE 0
#define CONSUMER_PRIORITY 4

/* Get a new value from the queue and set it as the value for the GATT characteristic */
void update_potentiometer_value(void *pQueue);

/* Public function to get the current value for the potentiometer (used by GATT service) */
uint16_t get_potentiometer_value(void);

/* Initialize consumer task */
void potentiometer_data_consumer_init(void *pQueue);

#endif  // CONSUMER_H