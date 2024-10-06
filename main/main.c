#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_nimble_cfg.h"

#define GENERIC_TAG_APPEARANCE (0x0200)

static uint8_t own_addr_type;
static uint8_t addr_val[6] ; 
static uint8_t esp_uri[] = {0x17, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};
static const char* TAG = "nimBLE";
void ble_advertise(void);
inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}
int gap_init(void)
{
    ble_svc_gap_init();

    int rc =  ble_svc_gap_device_name_set("ESP32_BLE");
    if(rc != 0)
    {
        ESP_LOGE(TAG,"device name set error!");
        return rc;
    }
    rc = ble_svc_gap_device_appearance_set(GENERIC_TAG_APPEARANCE);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
        return rc;
    }
    return rc;
}
 void ble_advertise(void)
{
    int rc = 0;
    const char* name;
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Set device name */
    name = ble_svc_gap_device_name();
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;
    
    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    /* Set device appearance */
    adv_fields.appearance = GENERIC_TAG_APPEARANCE;
    adv_fields.appearance_is_present = 1;

    /* Set device LE role */
    adv_fields.le_role = 0;
    adv_fields.le_role_is_present = 1;

    /* Set advertiement fields */
    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    /* Set device address */
    rsp_fields.device_addr = addr_val;
    rsp_fields.device_addr_type = own_addr_type;
    rsp_fields.device_addr_is_present = 1;
    rsp_fields.mfg_data = (uint8_t*)"00Hello World";
    rsp_fields.mfg_data_len = 13;
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
        return;
    }

    /* Set non-connectable and general discoverable mode to be a beacon */
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    /* Start advertising */
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                           NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    } else ESP_LOGI(TAG, "advertising started!");
}
void adv_init(void)
{
     int rc = 0;
    char addr_str[18] = {0};

    /* Make sure we have proper BT identity address set */
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    /* Figure out BT address to use while advertising */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    /* Copy device address to addr_val */
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, addr_val);
    ESP_LOGI(TAG, "device address: %s", addr_str);

    /* Start advertising. */
    ble_advertise();
}

static void on_stack_reset(int reason)
{
    ESP_LOGI(TAG,"nimBLE stack reset, reason: %d", reason);
}
static void on_stack_sync(void)
{
    adv_init();
    ESP_LOGI(TAG,"nimBLE stack sync");
}
static void nimble_host_init(void)
{
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_arg = ble_store_util_status_rr;
}

void ble_host_task(void* pvArg)
{
    nimble_port_run();

    vTaskDelete(NULL);
}

void app_main(void)
{
    nvs_flash_init();
    
    /*Init NimBLE stack*/
    nimble_port_init();
   
    gap_init();
    nimble_host_init();
    xTaskCreate(ble_host_task, "ble_host_task", 4096, NULL, 5, NULL);
}