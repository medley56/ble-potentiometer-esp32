#ifndef BLE_H
#define BLE_H

#define BLE_NOTIFICATION_PERIOD_MS 500 // 500ms (2Hz). Slow because BLE is slow

/* Initialize the BLE system, including the task that sends notifications to connected devices */
void ble_init(void);

#endif // BLE_H