/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gatt_svc.h"
#include "common.h"

/* Private function declarations */
static int potentiometer_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Volume Control Service (potentiometer is just a dial) */
static const ble_uuid16_t potentiometer_svc_uuid = BLE_UUID16_INIT(0x1844);

static uint8_t potentiometer_chr_val[2] = {0};
static uint16_t potentiometer_chr_val_handle;
static const ble_uuid16_t potentiometer_chr_uuid = BLE_UUID16_INIT(0x2B7D);

static uint16_t potentiometer_chr_conn_handle = 0;
static bool potentiometer_chr_conn_handle_inited = false;
static bool potentiometer_ind_status = false;

/* Custom GATT Services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Potentiometer service */
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &potentiometer_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {/* Potentiometer characteristic */
              .uuid = &potentiometer_chr_uuid.u,
              .access_cb = potentiometer_chr_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
              .val_handle = &potentiometer_chr_val_handle},
             {
                 0, /* No more characteristics in this service. */
             }}},

    {
        0, /* No more services. */
    },
};

/* Private functions */
static int potentiometer_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    /* Note: Potentiometer characteristic is read only */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == potentiometer_chr_val_handle) {
            /* Update access buffer value */
            potentiometer_chr_val[1] = 16;
            rc = os_mbuf_append(ctxt->om, &potentiometer_chr_val,
                                sizeof(potentiometer_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to potentiometer characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */
void send_potentiometer_indication(void) {
    if (potentiometer_ind_status && potentiometer_chr_conn_handle_inited) {
        ble_gatts_indicate(potentiometer_chr_conn_handle,
                           potentiometer_chr_val_handle);
        ESP_LOGI(TAG, "potentiometer indication sent!");
    }
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update potentiometer subscription status
 */

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == potentiometer_chr_val_handle) {
        /* Update potentiometer subscription status */
        potentiometer_chr_conn_handle = event->subscribe.conn_handle;
        potentiometer_chr_conn_handle_inited = true;
        potentiometer_ind_status = event->subscribe.cur_indicate;
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization (don't mess with this)
     * 
     * This initializes the standard GATT Service within the GATT server
     * Assigned value for the service is 0x1801 (and recall GAP is 0x1800).
     * This GATT Service includes the following characteristics:
     * - Service Changed (0x2a05)
     * - Server Supported Features (0x2b3a)
     * - Client Supported Features (0x2b29)
     */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}