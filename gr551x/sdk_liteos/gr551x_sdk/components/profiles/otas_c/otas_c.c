/**
 *****************************************************************************************
 *
 * @file otas_c.c
 *
 * @brief OTA Client Implementation.
 *
 *****************************************************************************************
 * @attention
  #####Copyright (c) 2019 GOODIX
  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of GOODIX nor the names of its contributors may be used
    to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 *****************************************************************************************
 */
#include "otas_c.h"
#include <string.h>

#define ATTR_VALUE_LEN 2
#define UUID_LEN_16 16
/*
 * STRUCT DEFINE
 *****************************************************************************************
 */
/**@brief OTA Service Client environment variable. */
struct otas_c_env_t {
    otas_c_handles_t      handles;            /**< Handles of OTA characteristics which will be got for peer. */
    otas_c_evt_handler_t  evt_handler;        /**< Handler of OTA client event  */
    uint8_t               prf_id;             /**< OTA Client profile id. */
};

/*
 * LOCAL FUNCTION DECLARATION
 *****************************************************************************************
 */
static void otas_c_att_write_cb(uint8_t conn_idx, uint8_t status, uint16_t handle);
static void otas_c_att_ntf_ind_cb(uint8_t conn_idx, const ble_gattc_ntf_ind_t *p_ntf_ind);
static void otas_c_srvc_browse_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_browse_srvc_t *p_browse_srvc);

/*
 * LOCAL VARIABLE DEFINITIONS
 *****************************************************************************************
 */
static struct otas_c_env_t s_otas_c_env;
static uint8_t            s_otas_uuid[16]              = OTAS_SVC_UUID;
static uint8_t            s_otas_tx_char_uuid[16]      = OTAS_TX_CHAR_UUID;
static uint8_t            s_otas_rx_char_uuid[16]      = OTAS_RX_CHAR_UUID;
static uint8_t            s_otas_ctrl_char_uuid[16]    = OTAS_CTRL_CHAR_UUID;


/**@brief OTA Client interface required by profile manager. */
static ble_prf_manager_cbs_t otas_c_mgr_cbs = {
    NULL,
    NULL,
    NULL
};

/**@brief OTA GATT Client Callbacks. */
static gattc_prf_cbs_t otas_c_gattc_cbs = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    otas_c_att_write_cb,
    otas_c_att_ntf_ind_cb,
    otas_c_srvc_browse_cb,
    NULL,
};

/**@brief OTA Client Information. */
static const prf_client_info_t otas_c_prf_info = {
    .max_connection_nb = OTAS_C_CONNECTION_MAX,
    .manager_cbs       = &otas_c_mgr_cbs,
    .gattc_prf_cbs     = &otas_c_gattc_cbs
};

/*
 * LOCAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */
/**
 *****************************************************************************************
 * @brief Execute OTA Service Client event handler.
 *
 * @param[in] p_evt: Pointer to OTA Service Client event structure.
 *****************************************************************************************
 */
static void otas_c_evt_handler_execute(otas_c_evt_t *p_evt)
{
    if (s_otas_c_env.evt_handler != NULL && OTAS_C_EVT_INVALID != p_evt->evt_type) {
        s_otas_c_env.evt_handler(p_evt);
    }
}

/**
 *****************************************************************************************
 * @brief This callback function will be called when receiving read response.
 *
 * @param[in] conn_idx:   The connection index.
 * @param[in] status:     The status of GATTC operation.
 * @param[in] handle:     The handle of attribute.
 *****************************************************************************************
 */
static void otas_c_att_write_cb(uint8_t conn_idx, uint8_t status, uint16_t handle)
{
    otas_c_evt_t otas_c_evt;

    otas_c_evt.conn_idx = conn_idx;
    otas_c_evt.evt_type = OTAS_C_EVT_INVALID;

    if (handle == s_otas_c_env.handles.otas_tx_cccd_handle) {
        otas_c_evt.evt_type = (BLE_SUCCESS == status) ? \
                              OTAS_C_EVT_TX_NTF_SET_SUCCESS : \
                              OTAS_C_EVT_WRITE_OP_ERR;
    } else if (handle == s_otas_c_env.handles.otas_ctrl_handle) {
        otas_c_evt.evt_type = (BLE_SUCCESS == status) ? \
                              OTAS_C_EVT_CTRL_SUCCESS : \
                              OTAS_C_EVT_WRITE_OP_ERR;
    } else if (handle == s_otas_c_env.handles.otas_rx_handle) {
        otas_c_evt.evt_type = (BLE_SUCCESS == status) ? \
                              OTAS_C_EVT_TX_CPLT : \
                              OTAS_C_EVT_WRITE_OP_ERR;
    }

    otas_c_evt_handler_execute(&otas_c_evt);
}

/**
 *****************************************************************************************
 * @brief This callback function will be called when receiving notification or indication.
 *
 * @param[in] conn_idx:  The connection index.
 * @param[in] status:    The status of GATTC operation.
 * @param[in] p_ntf_ind: The information of notification or indication.
 *****************************************************************************************
 */
static void otas_c_att_ntf_ind_cb(uint8_t conn_idx, const ble_gattc_ntf_ind_t *p_ntf_ind)
{
    otas_c_evt_t otas_c_evt;

    otas_c_evt.conn_idx = conn_idx;
    otas_c_evt.evt_type = OTAS_C_EVT_INVALID;
    otas_c_evt.p_data   = p_ntf_ind->p_value;
    otas_c_evt.length   = p_ntf_ind->length;

    if (p_ntf_ind->handle == s_otas_c_env.handles.otas_tx_handle) {
        otas_c_evt.evt_type = OTAS_C_EVT_PEER_DATA_RECEIVE;
    }

    otas_c_evt_handler_execute(&otas_c_evt);
}

/**
 *****************************************************************************************
 * @brief This callback function will be called when receiving browse service indication.
 *
 * @param[in] conn_idx:      The connection index.
 * @param[in] status:        The status of GATTC operation.
 * @param[in] p_browse_srvc: The information of service browse.
 *****************************************************************************************
 */
static void otas_c_srvc_browse_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_browse_srvc_t *p_browse_srvc)
{
    otas_c_evt_t  otas_c_evt;
    uint16_t     handle_disc;

    otas_c_evt.conn_idx = conn_idx;
    otas_c_evt.evt_type = OTAS_C_EVT_DISCOVERY_FAIL;

    if (BLE_GATT_ERR_BROWSE_NO_ANY_MORE == status) {
        return;
    }

    if (status != BLE_SUCCESS) {
        return;
    }

    if (p_browse_srvc->uuid_len == UUID_LEN_16 && memcmp(p_browse_srvc->uuid, s_otas_uuid, UUID_LEN_16) == 0) {
        s_otas_c_env.handles.otas_srvc_start_handle = p_browse_srvc->start_hdl;
        s_otas_c_env.handles.otas_srvc_end_handle   = p_browse_srvc->end_hdl;

        for (uint32_t i = 0; i < (p_browse_srvc->end_hdl - p_browse_srvc->start_hdl); i++) {
            handle_disc = p_browse_srvc->start_hdl + i + 1;

            if (p_browse_srvc->info[i].attr_type == BLE_GATTC_BROWSE_ATTR_VAL) {
                if (memcmp(p_browse_srvc->info[i].attr.uuid, s_otas_rx_char_uuid, UUID_LEN_16) == 0) {
                    s_otas_c_env.handles.otas_rx_handle = handle_disc;
                } else if ((memcmp(p_browse_srvc->info[i].attr.uuid, s_otas_tx_char_uuid, UUID_LEN_16) == 0)) {
                    s_otas_c_env.handles.otas_tx_handle      = handle_disc;
                    s_otas_c_env.handles.otas_tx_cccd_handle = handle_disc + 1;
                } else if (memcmp(p_browse_srvc->info[i].attr.uuid, s_otas_ctrl_char_uuid, UUID_LEN_16) == 0) {
                    s_otas_c_env.handles.otas_ctrl_handle = handle_disc;
                }
            }

            if (p_browse_srvc->info[i].attr_type == BLE_GATTC_BROWSE_NONE) {
                break;
            }
        }

        otas_c_evt.evt_type = OTAS_C_EVT_DISCOVERY_COMPLETE;
    }

    otas_c_evt_handler_execute(&otas_c_evt);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */
sdk_err_t otas_client_init(otas_c_evt_handler_t evt_handler)
{
    sdk_err_t ret;
    if (evt_handler == NULL) {
        return SDK_ERR_POINTER_NULL;
    }

    ret = memset_s(&s_otas_c_env, sizeof(s_otas_c_env), 0, sizeof(s_otas_c_env));
    if (ret < 0) {
        return ret;
    }
    s_otas_c_env.evt_handler = evt_handler;

    return ble_client_prf_add(&otas_c_prf_info, &s_otas_c_env.prf_id);
}


sdk_err_t otas_c_disc_srvc_start(uint8_t conn_idx)
{
    const ble_uuid_t ths_uuid = {
        .uuid_len = 16,
        .uuid     = s_otas_uuid,
    };

    return ble_gattc_prf_services_browse(s_otas_c_env.prf_id, conn_idx, &ths_uuid);
}


sdk_err_t otas_c_tx_notify_set(uint8_t conn_idx, bool is_enable)
{
    gattc_write_attr_value_t write_attr_value;
    uint16_t ntf_value = is_enable ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    if (BLE_ATT_INVALID_HDL == s_otas_c_env.handles.otas_tx_cccd_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    write_attr_value.handle  = s_otas_c_env.handles.otas_tx_cccd_handle;
    write_attr_value.offset  = 0;
    write_attr_value.length  = ATTR_VALUE_LEN;
    write_attr_value.p_value = (uint8_t *)&ntf_value;

    return ble_gattc_prf_write(s_otas_c_env.prf_id, conn_idx, &write_attr_value);
}

sdk_err_t otas_c_ctrl_data_send(uint8_t conn_idx, uint32_t data)
{
    gattc_write_no_resp_t write_attr_value;

    if (BLE_ATT_INVALID_HDL == s_otas_c_env.handles.otas_ctrl_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    write_attr_value.signed_write  = false;
    write_attr_value.handle        = s_otas_c_env.handles.otas_ctrl_handle;
    write_attr_value.length        = sizeof(uint32_t);
    write_attr_value.p_value       = (uint8_t*)&data;

    return ble_gattc_prf_write_no_resp(s_otas_c_env.prf_id, conn_idx, &write_attr_value);
}

sdk_err_t otas_c_tx_data_send(uint8_t conn_idx, uint8_t *p_data, uint16_t length)
{
    gattc_write_no_resp_t write_attr_value;

    if (BLE_ATT_INVALID_HDL == s_otas_c_env.handles.otas_rx_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    write_attr_value.signed_write  = false;
    write_attr_value.handle        = s_otas_c_env.handles.otas_rx_handle;
    write_attr_value.length        = length;
    write_attr_value.p_value       = p_data;

    return ble_gattc_prf_write_no_resp(s_otas_c_env.prf_id, conn_idx, &write_attr_value);
}
