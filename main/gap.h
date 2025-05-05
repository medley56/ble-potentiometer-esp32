/* Module for configuring and running a BLE Generic Access Profile (GAP)
 */
#ifndef GAP_SVC_H
#define GAP_SVC_H

/* Includes */
/* NimBLE GAP APIs */
#include "host/ble_gap.h"
#include "services/gap/ble_svc_gap.h"

/* Defines */
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

/* Function to start advertising 

This function configures the advertisement packet contents, 
the scan response contents, and the advertising process itself.
*/
void adv_init(void);

/* Function to initialize GAP 

This function creates a GATT service called the "GAP Service", which 
contains the standard characteristics of Appearance and Device Name, 
allowing central devices to see what they are connecting to.
*/
int gap_init(void);

#endif // GAP_SVC_H