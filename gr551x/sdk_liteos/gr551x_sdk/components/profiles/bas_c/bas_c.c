/**
 *****************************************************************************************
 *
 * @file bas_c.c
 *
 * @brief Battery Service Client Implementation.
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
#include <string.h>
#include "ble_prf_utils.h"
#include "utility.h"
#include "bas_c.h"

#define UUID_OFFSET_8 8
#define ATTR_VALUE_LEN 2
/*
 * STRUCT DEFINE
 *****************************************************************************************
 */
/**@brief Battery Service environment variable. */
struct bas_c_env_t {
    bas_c_handles_t     handles;            /**< Handles of BAS characteristics which will be got for peer. */
    bas_c_evt_handler_t evt_handler;        /**< Handler of BAS Client event  */
    uint8_t             prf_id;             /**< BAS Client profile id. */
};

/*
 * LOCAL FUNCTION DECLARATION
 *****************************************************************************************
 */
static void bas_c_att_read_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_read_rsp_t *p_read_rsp);
static void bas_c_att_write_cb(uint8_t conn_idx, uint8_t status, uint16_t handle);
static void bas_c_att_ntf_ind_cb(uint8_t conn_idx, const ble_gattc_ntf_ind_t *p_ntf_ind);
static void bas_c_srvc_browse_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_browse_srvc_t *p_browse_srvc);

/*
 * LOCAL VARIABLE DEFINITIONS
 *****************************************************************************************
 */
static struct bas_c_env_t s_bas_c_env;     /**< Battery Service Client environment variable. */

/**@brief Battery Service Client interface required by profile manager. */
static ble_prf_manager_cbs_t bas_c_mgr_cbs = {
    NULL,
    NULL,
    NULL
};

/**@brief Battery Service GATT Client Callbacks. */
static gattc_prf_cbs_t bas_c_gattc_cbs = {
    NULL,
    NULL,
    NULL,
    NULL,
    bas_c_att_read_cb,
    bas_c_att_write_cb,
    bas_c_att_ntf_ind_cb,
    bas_c_srvc_browse_cb,
    NULL,
};

/**@brief Battery Service Client Information. */
static const prf_client_info_t bas_c_prf_info = {
    .max_connection_nb = BAS_C_CONNECTION_MAX,
    .manager_cbs       = &bas_c_mgr_cbs,
    .gattc_prf_cbs     = &bas_c_gattc_cbs
};

/*
 * LOCAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */
/**
 *****************************************************************************************
 * @brief Excute Battery Service Client event handler.
 *
 * @param[in] p_evt: Pointer to Battery Service Client event structure.
 *****************************************************************************************
 */
void bas_c_evt_handler_excute(bas_c_evt_t *p_evt)
{
    if (s_bas_c_env.evt_handler != NULL && BAS_C_EVT_INVALID != p_evt->evt_type) {
        s_bas_c_env.evt_handler(p_evt);
    }
}

/**
 *****************************************************************************************
 * @brief This callback function will be called when receiving read response.
 *
 * @param[in] conn_idx:   The connection index.
 * @param[in] status:     The status of GATTC operation.
 * @param[in] p_read_rsp: The information of read response.
 *****************************************************************************************
 */
static void bas_c_att_read_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_read_rsp_t *p_read_rsp)
{
    bas_c_evt_t bas_c_evt;

    bas_c_evt.conn_idx = conn_idx;
    bas_c_evt.evt_type = BAS_C_EVT_INVALID;

    if (BLE_SUCCESS != status) {
        return;
    }

    if (p_read_rsp->vals[0].handle == s_bas_c_env.handles.bas_bat_level_handle) {
        bas_c_evt.evt_type  = BAS_C_EVT_BAT_LEVE_RECEIVE;
        bas_c_evt.bat_level = p_read_rsp->vals[0].p_value[0];
        bas_c_evt_handler_excute(&bas_c_evt);
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
static void bas_c_att_write_cb(uint8_t conn_idx, uint8_t status, uint16_t handle)
{
    bas_c_evt_t bas_c_evt;

    bas_c_evt.conn_idx = conn_idx;
    bas_c_evt.evt_type = BAS_C_EVT_INVALID;

    if (handle == s_bas_c_env.handles.bas_bat_level_cccd_handle) {
        bas_c_evt.evt_type = (BLE_SUCCESS == status) ?\
                             BAS_C_EVT_BAT_LEVEL_NTF_SET_SUCCESS :\
                             BAS_C_EVT_BAT_LEVEL_NTF_SET_ERR;
    }

    bas_c_evt_handler_excute(&bas_c_evt);
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
static void bas_c_att_ntf_ind_cb(uint8_t conn_idx, const ble_gattc_ntf_ind_t *p_ntf_ind)
{
    bas_c_evt_t bas_c_evt;

    bas_c_evt.conn_idx = conn_idx;
    bas_c_evt.evt_type = BAS_C_EVT_INVALID;

    if (p_ntf_ind->handle == s_bas_c_env.handles.bas_bat_level_handle) {
        bas_c_evt.evt_type  = BAS_C_EVT_BAT_LEVE_RECEIVE;
        bas_c_evt.bat_level = p_ntf_ind->p_value[0];
        bas_c_evt_handler_excute(&bas_c_evt);
    }
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
static void bas_c_srvc_browse_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_browse_srvc_t *p_browse_srvc)
{
    bas_c_evt_t  bas_c_evt;
    uint16_t     uuid_disc;
    uint16_t     handle_disc;

    bas_c_evt.conn_idx = conn_idx;
    bas_c_evt.evt_type = BAS_C_EVT_DISCOVERY_FAIL;

    if (BLE_GATT_ERR_BROWSE_NO_ANY_MORE == status) {
        return;
    }

    if (status != BLE_SUCCESS) {
        return;
    }
    uuid_disc = p_browse_srvc->uuid[0] | p_browse_srvc->uuid[1] << UUID_OFFSET_8;

    if (BLE_ATT_SVC_BATTERY_SERVICE == uuid_disc) {
        s_bas_c_env.handles.bas_srvc_start_handle = p_browse_srvc->start_hdl;
        s_bas_c_env.handles.bas_srvc_end_handle   = p_browse_srvc->end_hdl;

        for (uint32_t i = 0; i < (p_browse_srvc->end_hdl - p_browse_srvc->start_hdl); i++) {
            uuid_disc   = p_browse_srvc->info[i].attr.uuid[0] |
                          p_browse_srvc->info[i].attr.uuid[1] << UUID_OFFSET_8;
            handle_disc = p_browse_srvc->start_hdl + i + 1;

            if (BLE_GATTC_BROWSE_ATTR_VAL == p_browse_srvc->info[i].attr_type) {
                if (BLE_ATT_CHAR_BATTERY_LEVEL == uuid_disc) {
                    s_bas_c_env.handles.bas_bat_level_handle = handle_disc;
                }
            } else if (BLE_GATTC_BROWSE_ATTR_DESC == p_browse_srvc->info[i].attr_type) {
                if (BLE_ATT_DESC_CLIENT_CHAR_CFG == uuid_disc) {
                    s_bas_c_env.handles.bas_bat_level_cccd_handle = handle_disc;
                } else if (BLE_ATT_DESC_CHAR_PRES_FORMAT == uuid_disc) {
                    s_bas_c_env.handles.bas_bat_level_pres_handle = handle_disc;
                }
            } else if (BLE_GATTC_BROWSE_NONE == p_browse_srvc->info[i].attr_type) {
                break;
            }
        }

        bas_c_evt.evt_type = BAS_C_EVT_DISCOVERY_COMPLETE;
    }

    bas_c_evt_handler_excute(&bas_c_evt);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */
sdk_err_t bas_client_init(bas_c_evt_handler_t evt_handler)
{
    sdk_err_t ret;
    if (evt_handler == NULL) {
        return SDK_ERR_POINTER_NULL;
    }

    ret = memset_s(&s_bas_c_env, sizeof(s_bas_c_env), 0, sizeof(s_bas_c_env));
    if (ret < 0) {
        return ret;
    }
    s_bas_c_env.evt_handler = evt_handler;

    return ble_client_prf_add(&bas_c_prf_info, &s_bas_c_env.prf_id);
}

sdk_err_t bas_c_disc_srvc_start(uint8_t conn_idx)
{
    uint8_t target_uuid[2];

    target_uuid[0] = LO_U16(BLE_ATT_SVC_BATTERY_SERVICE);
    target_uuid[1] = HI_U16(BLE_ATT_SVC_BATTERY_SERVICE);

    const ble_uuid_t bas_service_uuid = {
        .uuid_len = 2,
        .uuid     = target_uuid,
    };

    return ble_gattc_prf_services_browse(s_bas_c_env.prf_id, conn_idx, &bas_service_uuid);
}

sdk_err_t bas_c_bat_level_notify_set(uint8_t conn_idx, bool is_enable)
{
    gattc_write_attr_value_t write_attr_value;
    uint16_t ntf_value = is_enable ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    if (BLE_ATT_INVALID_HDL == s_bas_c_env.handles.bas_bat_level_cccd_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    write_attr_value.handle  = s_bas_c_env.handles.bas_bat_level_cccd_handle;
    write_attr_value.offset  = 0;
    write_attr_value.length  = ATTR_VALUE_LEN;
    write_attr_value.p_value = (uint8_t *)&ntf_value;

    return ble_gattc_prf_write(s_bas_c_env.prf_id, conn_idx, &write_attr_value);
}

sdk_err_t bas_c_bat_level_read(uint8_t conn_idx)
{
    if (BLE_ATT_INVALID_HDL == s_bas_c_env.handles.bas_bat_level_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    return  ble_gattc_prf_read(s_bas_c_env.prf_id, conn_idx, s_bas_c_env.handles.bas_bat_level_handle, 0);
}
