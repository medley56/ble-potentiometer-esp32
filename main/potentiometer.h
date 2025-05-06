#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <inttypes.h>

#define QUEUE_WAIT_TICKS pdMS_TO_TICKS(200)  // Time to wait when retrieving from queue

void update_potentiometer_value(void *pQueue);

uint16_t get_potentiometer_value(void);

#endif  // POTENTIOMETER_H