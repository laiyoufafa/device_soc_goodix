/**
 *****************************************************************************************
 *
 * @file hrs_c.c
 *
 * @brief Heart Rate Service Client Implementation.
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
#include "hrs_c.h"
#define OFFSET_8 8
#define ATTR_VALUE_LEN 2
/*
 * STRUCT DEFINE
 *****************************************************************************************
 */
/**@brief Heart Rate Service environment variable. */
struct hrs_c_env_t {
    hrs_c_handles_t     handles;            /**< Handles of HRS characteristics which will be got for peer. */
    hrs_c_evt_handler_t evt_handler;        /**< Handler of HRS Client event  */
    uint8_t             prf_id;             /**< HRS Client profile id. */
};

/*
 * LOCAL FUNCTION DECLARATION
 *****************************************************************************************
 */
static void hrs_c_att_read_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_read_rsp_t *p_read_rsp);
static void hrs_c_att_write_cb(uint8_t conn_idx, uint8_t status, uint16_t handle);
static void hrs_c_att_ntf_ind_cb(uint8_t conn_idx, const ble_gattc_ntf_ind_t *p_ntf_ind);
static void hrs_c_srvc_browse_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_browse_srvc_t *p_browse_srvc);

/*
 * LOCAL VARIABLE DEFINITIONS
 *****************************************************************************************
 */
static struct hrs_c_env_t s_hrs_c_env;     /**< Heart Rate Service Client environment variable. */

/**@brief Heart Rate Service Client interface required by profile manager. */
static ble_prf_manager_cbs_t hrs_c_mgr_cbs = {
    NULL,
    NULL,
    NULL
};

/**@brief Heart Rate Service GATT Client Callbacks. */
static gattc_prf_cbs_t hrs_c_gattc_cbs = {
    NULL,
    NULL,
    NULL,
    NULL,
    hrs_c_att_read_cb,
    hrs_c_att_write_cb,
    hrs_c_att_ntf_ind_cb,
    hrs_c_srvc_browse_cb,
    NULL,
};

/**@brief Heart Rate Service Client Information. */
static const prf_client_info_t hrs_c_prf_info = {
    .max_connection_nb = HRS_C_CONNECTION_MAX,
    .manager_cbs       = &hrs_c_mgr_cbs,
    .gattc_prf_cbs     = &hrs_c_gattc_cbs
};

/*
 * LOCAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */
/**
 *****************************************************************************************
 * @brief Excute Heart Rate Service Client event handler.
 *
 * @param[in] p_evt: Pointer to Heart Rate Service Client event structure.
 *****************************************************************************************
 */
static void hrs_c_evt_handler_excute(hrs_c_evt_t *p_evt)
{
    if (s_hrs_c_env.evt_handler != NULL && HRS_C_EVT_INVALID != p_evt->evt_type) {
        s_hrs_c_env.evt_handler(p_evt);
    }
}

/**
 *****************************************************************************************
 * @brief Encode heart rate measurement value.
 *
 * @param[in]  p_data:         Pointer to data encode.
 * @param[in]  length:         Length of data encode.
 * @param[out] p_hr_meas_buff: Pointer to buffer for encode heart rate measurement value.
 *****************************************************************************************
 */
static void hrs_c_meas_value_encode(uint8_t *p_data, uint16_t length, hrs_c_hr_meas_t *p_hr_meas_buff)
{
    uint8_t   flags = 0;
    uint8_t   index = 0;
    uint8_t ret;

    flags = p_data[index++];

    ret = memset_s(p_hr_meas_buff, sizeof(hrs_c_hr_meas_t), 0, sizeof(hrs_c_hr_meas_t));
    if (ret < 0) {
        return ret;
    }

    if (flags & HRS_C_BIT_RATE_FORMAT) {
        p_hr_meas_buff->hr_value = p_data[index] | (p_data[index + 1] << OFFSET_8);
        index += sizeof(uint16_t);
    } else {
        p_hr_meas_buff->hr_value = p_data[index++];
    }

    if (flags & HRS_C_BIT_SENSOR_CONTACT_SUPPORTED) {
        if (flags & HRS_C_BIT_SENSOR_CONTACT_DETECTED) {
            p_hr_meas_buff->is_sensor_contact_detected = true;
        } else {
            p_hr_meas_buff->is_sensor_contact_detected = false;
        }
    }

    if (flags & HRS_C_BIT_ENERGY_EXPENDED_STATUS) {
        p_hr_meas_buff->energy_expended = p_data[index] | (p_data[index + 1] << OFFSET_8);
        index += sizeof(uint16_t);
    }

    if (flags & HRS_C_BIT_INTERVAL) {
        for (uint8_t i = 0; index < length; i++) {
            p_hr_meas_buff->rr_intervals_num++;
            p_hr_meas_buff->rr_intervals[i]  = (p_data[index] | (p_data[index + 1] << OFFSET_8)) * 1000 / 1024.0;
            index += sizeof(uint16_t);
        }
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
static void hrs_c_att_read_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_read_rsp_t *p_read_rsp)
{
    hrs_c_evt_t hrs_c_evt;

    hrs_c_evt.conn_idx = conn_idx;
    hrs_c_evt.evt_type = HRS_C_EVT_INVALID;

    if (BLE_SUCCESS != status) {
        return;
    }

    if (p_read_rsp->vals[0].handle == s_hrs_c_env.handles.hrs_sensor_loc_handle) {
        hrs_c_evt.evt_type         = HRS_C_EVT_SENSOR_LOC_READ_RSP;
        hrs_c_evt.value.sensor_loc = (hrs_c_sensor_loc_t)p_read_rsp->vals[0].p_value[0];
    }

    hrs_c_evt_handler_excute(&hrs_c_evt);
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
static void hrs_c_att_write_cb(uint8_t conn_idx, uint8_t status, uint16_t handle)
{
    hrs_c_evt_t hrs_c_evt;

    hrs_c_evt.conn_idx = conn_idx;
    hrs_c_evt.evt_type = HRS_C_EVT_INVALID;

    if (handle == s_hrs_c_env.handles.hrs_hr_meas_cccd_handle) {
        hrs_c_evt.evt_type = (BLE_SUCCESS == status) ? \
                             HRS_C_EVT_HR_MEAS_NTF_SET_SUCCESS :
                             HRS_C_EVT_WRITE_OP_ERR;
    } else if (handle == s_hrs_c_env.handles.hrs_ctrl_point_handle) {
        hrs_c_evt.evt_type = (BLE_SUCCESS == status) ? \
                             HRS_C_EVT_CTRL_POINT_SET :
                             HRS_C_EVT_WRITE_OP_ERR;
    }

    hrs_c_evt_handler_excute(&hrs_c_evt);
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
static void hrs_c_att_ntf_ind_cb(uint8_t conn_idx, const ble_gattc_ntf_ind_t *p_ntf_ind)
{
    hrs_c_evt_t hrs_c_evt;

    hrs_c_evt.conn_idx = conn_idx;
    hrs_c_evt.evt_type = HRS_C_EVT_INVALID;

    if (p_ntf_ind->handle == s_hrs_c_env.handles.hrs_hr_meas_handle) {
        hrs_c_evt.evt_type = HRS_C_EVT_HR_MEAS_VAL_RECEIVE;
        hrs_c_meas_value_encode(p_ntf_ind->p_value, p_ntf_ind->length, &hrs_c_evt.value.hr_meas_buff);
    }

    hrs_c_evt_handler_excute(&hrs_c_evt);
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
static void hrs_c_srvc_browse_cb(uint8_t conn_idx, uint8_t status, const ble_gattc_browse_srvc_t *p_browse_srvc)
{
    hrs_c_evt_t  hrs_c_evt;
    uint16_t     uuid_disc;
    uint16_t     handle_disc;

    hrs_c_evt.conn_idx = conn_idx;
    hrs_c_evt.evt_type = HRS_C_EVT_DISCOVERY_FAIL;

    if (BLE_GATT_ERR_BROWSE_NO_ANY_MORE == status) {
        return;
    }

    if (status != BLE_SUCCESS) {
        return;
    }
    uuid_disc = p_browse_srvc->uuid[0] | p_browse_srvc->uuid[1] << OFFSET_8;

    if (BLE_ATT_SVC_HEART_RATE == uuid_disc) {
        s_hrs_c_env.handles.hrs_srvc_start_handle = p_browse_srvc->start_hdl;
        s_hrs_c_env.handles.hrs_srvc_end_handle   = p_browse_srvc->end_hdl;

        for (uint32_t i = 0; i < (p_browse_srvc->end_hdl - p_browse_srvc->start_hdl); i++) {
            uuid_disc   = p_browse_srvc->info[i].attr.uuid[0] | p_browse_srvc->info[i].attr.uuid[1] << OFFSET_8;
            handle_disc = p_browse_srvc->start_hdl + i + 1;

            if (BLE_GATTC_BROWSE_ATTR_VAL == p_browse_srvc->info[i].attr_type) {
                if (BLE_ATT_CHAR_HEART_RATE_MEAS == uuid_disc) {
                    s_hrs_c_env.handles.hrs_hr_meas_handle = handle_disc;
                } else if (BLE_ATT_CHAR_BODY_SENSOR_LOCATION == uuid_disc) {
                    s_hrs_c_env.handles.hrs_sensor_loc_handle = handle_disc;
                } else if (BLE_ATT_CHAR_HEART_RATE_CNTL_POINT == uuid_disc) {
                    s_hrs_c_env.handles.hrs_ctrl_point_handle = handle_disc;
                }
            } else if (BLE_GATTC_BROWSE_ATTR_DESC == p_browse_srvc->info[i].attr_type) {
                if (BLE_ATT_DESC_CLIENT_CHAR_CFG == uuid_disc) {
                    s_hrs_c_env.handles.hrs_hr_meas_cccd_handle = handle_disc;
                }
            } else if (BLE_GATTC_BROWSE_NONE == p_browse_srvc->info[i].attr_type) {
                break;
            }
        }

        hrs_c_evt.evt_type = HRS_C_EVT_DISCOVERY_COMPLETE;
    }

    hrs_c_evt_handler_excute(&hrs_c_evt);
}


/*
 * GLOBAL FUNCTION DEFINITIONS
 *****************************************************************************************
 */
sdk_err_t hrs_client_init(hrs_c_evt_handler_t evt_handler)
{
    sdk_err_t ret;
    if (evt_handler == NULL) {
        return SDK_ERR_POINTER_NULL;
    }

    ret = memset_s(&s_hrs_c_env, sizeof(s_hrs_c_env), 0, sizeof(s_hrs_c_env));
    if (ret < 0) {
        return ret;
    }
    s_hrs_c_env.evt_handler = evt_handler;

    return ble_client_prf_add(&hrs_c_prf_info, &s_hrs_c_env.prf_id);
}

sdk_err_t hrs_c_disc_srvc_start(uint8_t conn_idx)
{
    uint8_t target_uuid[2];

    target_uuid[0] = LO_U16(BLE_ATT_SVC_HEART_RATE);
    target_uuid[1] = HI_U16(BLE_ATT_SVC_HEART_RATE);

    const ble_uuid_t hrs_service_uuid = {
        .uuid_len = 2,
        .uuid     = target_uuid,
    };

    return ble_gattc_prf_services_browse(s_hrs_c_env.prf_id, conn_idx, &hrs_service_uuid);
}

sdk_err_t hrs_c_heart_rate_meas_notify_set(uint8_t conn_idx, bool is_enable)
{
    gattc_write_attr_value_t write_attr_value;
    uint16_t ntf_value = is_enable ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    if (BLE_ATT_INVALID_HDL == s_hrs_c_env.handles.hrs_hr_meas_cccd_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    write_attr_value.handle  = s_hrs_c_env.handles.hrs_hr_meas_cccd_handle;
    write_attr_value.offset  = 0;
    write_attr_value.length  = ATTR_VALUE_LEN;
    write_attr_value.p_value = (uint8_t *)&ntf_value;

    return ble_gattc_prf_write(s_hrs_c_env.prf_id, conn_idx, &write_attr_value);
}

sdk_err_t hrs_c_sensor_loc_read(uint8_t conn_idx)
{
    if (BLE_ATT_INVALID_HDL == s_hrs_c_env.handles.hrs_sensor_loc_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    return ble_gattc_prf_read(s_hrs_c_env.prf_id, conn_idx, s_hrs_c_env.handles.hrs_sensor_loc_handle, 0);
}

sdk_err_t hrs_c_ctrl_point_set(uint8_t conn_idx, uint16_t ctrl_value)
{
    gattc_write_attr_value_t write_attr_value;

    if (BLE_ATT_INVALID_HDL == s_hrs_c_env.handles.hrs_ctrl_point_handle) {
        return SDK_ERR_INVALID_HANDLE;
    }

    write_attr_value.handle  = s_hrs_c_env.handles.hrs_ctrl_point_handle;
    write_attr_value.offset  = 0;
    write_attr_value.length  = ATTR_VALUE_LEN;
    write_attr_value.p_value = (uint8_t *)&ctrl_value;

    return ble_gattc_prf_write(s_hrs_c_env.prf_id, conn_idx, &write_attr_value);
}

