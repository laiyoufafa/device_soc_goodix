/**
 *****************************************************************************************
 *
 * @file hids.c
 *
 * @brief THe Implementation of Human Input Device Service.
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
#include "hids.h"
#include "ble_prf_types.h"
#include "ble_prf_utils.h"
#include "utility.h"
#define INDEX_0 0
#define INDEX_1 1
#define INDEX_2 2
#define INDEX_3 3
#define INDEX_4 4
/*
 * DEFINES
 *******************************************************************************
 */
#define IN_REPORT_MAX_COUNT                       3                  /**< Maximum support input report count. */
/**< Maximum size of a Boot Keyboard Input Report (as per Appendix B in Device Class Definition
 * for Human Interface Devices (HID), Version 1.11). */
#define HIDS_BOOT_KB_IN_REPORT_MAX_SIZE           8
/**< Maximum size of a Boot Keyboard Output Report (as per Appendix B in Device Class Definition
 * for Human Interface Devices (HID), Version 1.11). */
#define HIDS_BOOT_KB_OUT_REPORT_MAX_SIZE          1
/**< Minimum size of a Boot Mouse Input Report (as per Appendix B in Device Class Definition
 * for Human Interface Devices (HID), Version 1.11). */
#define HIDS_BOOT_MOUSE_IN_REPORT_MIN_SIZE        3
/**< Maximum size of a Boot Mouse Input Report (as per Appendix B in Device Class Definition
 * for Human Interface Devices (HID), Version 1.11). */
#define HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE        8


// Protocol Mode values
#define PROTOCOL_MODE_BOOT                        0x00               /**< Boot Protocol Mode. */
#define PROTOCOL_MODE_REPORT                      0x01               /**< Report Protocol Mode. */

// HID Control Point values
#define HIDS_CONTROL_POINT_SUSPEND                0x00               /**< Suspend command. */
#define HIDS_CONTROL_POINT_EXIT_SUSPEND           0x01               /**< Exit Suspend command. */
/**< Default value for the Protocol Mode characteristic. */
#define DEFAULT_PROTOCOL_MODE            PROTOCOL_MODE_REPORT
/**< Initial value for the HID Control Point characteristic. */
#define INITIAL_VALUE_HID_CONTROL_POINT  HIDS_CONTROL_POINT_SUSPEND
#define REPORT_CNT_0 0
#define REPORT_CNT_1 1
#define REPORT_CNT_2 2
/*
 * ENUMERATIONS
 *****************************************************************************************
 */
/**@brief HIDS Attributes database index list. */
enum hids_attr_idx_tag {
    // Service
    HIDS_IDX_SVC,

    // Protocol Mode characteristic
    HIDS_IDX_PROTOCOL_MODE_CHAR,
    HIDS_IDX_PROTOCOL_MODE_VAL,

    // Input Report1 characteristics
    HIDS_IDX_INPUT1_REPORT_CHAR,
    HIDS_IDX_INPUT1_REPORT_VAL,
    HIDS_IDX_INPUT1_REPORT_CCCD,
    HIDS_IDX_INPUT1_REPORT_REF,

    // Input Report2 characteristics
    HIDS_IDX_INPUT2_REPORT_CHAR,
    HIDS_IDX_INPUT2_REPORT_VAL,
    HIDS_IDX_INPUT2_REPORT_CCCD,
    HIDS_IDX_INPUT2_REPORT_REF,

    // Input Report3 characteristics
    HIDS_IDX_INPUT3_REPORT_CHAR,
    HIDS_IDX_INPUT3_REPORT_VAL,
    HIDS_IDX_INPUT3_REPORT_CCCD,
    HIDS_IDX_INPUT3_REPORT_REF,

    // Output Report characteristics
    HIDS_IDX_OUTPUT_REPORT_CHAR,
    HIDS_IDX_OUTPUT_REPORT_VAL,
    HIDS_IDX_OUTPUT_REPORT_REF,

    // Feature Report characteristic
    HIDS_IDX_FEATURE_REPORT_CHAR,
    HIDS_IDX_FEATURE_REPORT_VAL,
    HIDS_IDX_FEATURE_REPORT_REF,

    // Report Map characteristic
    HIDS_IDX_REPORT_MAP_CHAR,
    HIDS_IDX_REPORT_MAP_VAL,

    // Boot Keyboard Input Report characteristic
    HIDS_IDX_BOOT_KB_IN_RPT_CHAR,
    HIDS_IDX_BOOT_KB_IN_RPT_VAL,
    HIDS_IDX_BOOT_KB_IN_RPT_CCCD,

    // Boot Keyboard Output Report characteristic
    HIDS_IDX_BOOT_KB_OUT_RPT_CHAR,
    HIDS_IDX_BOOT_KB_OUT_RPT_VAL,

    // Boot Mouse Input Report characteristic.
    HIDS_IDX_BOOT_MS_IN_RPT_CHAR,
    HIDS_IDX_BOOT_MS_IN_RPT_VAL,
    HIDS_IDX_BOOT_MS_IN_RPT_CCCD,

    // HID Information characteristic
    HIDS_IDX_HID_INFO_CHAR,
    HIDS_IDX_HID_INFO_VAL,

    // HID Control Point characteristic
    HIDS_IDX_CTRL_POINT_CHAR,
    HIDS_IDX_CTRL_POINT_VAL,

    HIDS_IDX_NB,
};

/*
 * STRUCT DEFINE
 *******************************************************************************
 */
/**@brief Heart Rate Service environment variable. */
struct hids_env_t {
    hids_init_t               hids_init;                         /**< HID Service Init Value. */
    uint16_t                  start_hdl;                         /**< HID Service start handle. */
    uint8_t
    char_mask[5];                                                /**< Mask of Supported characteristics*/
    uint16_t
    input_cccd[IN_REPORT_MAX_COUNT][HIDS_CONNECTION_MAX];        /**< Input report characteristics cccd value*/
    uint16_t
    kb_input_cccd[HIDS_CONNECTION_MAX];            /**< Boot keyboard input report characteristics cccd value*/
    uint16_t
    mouse_input_cccd[HIDS_CONNECTION_MAX];         /**< Boot mouse input report characteristics cccd value*/
    uint8_t                   protocol_mode;                                               /**< Protocol mode. */
    uint8_t                   ctrl_pt;                                                     /**< HID Control Point. */
    uint8_t
    input_report_val[IN_REPORT_MAX_COUNT][HIDS_REPORT_MAX_SIZE]; /**< Input report characteristics value*/
    uint8_t
    output_report_val[HIDS_REPORT_MAX_SIZE];                     /**< Output report characteristic value*/
    uint8_t
    feature_report_val[HIDS_REPORT_MAX_SIZE];                    /**< Feature report characteristic value*/
    uint8_t
    kb_input_report_val[HIDS_BOOT_KB_IN_REPORT_MAX_SIZE];  /**< Boot keyboard input report characteristics value*/
    uint8_t
    kb_output_report_val[HIDS_BOOT_KB_OUT_REPORT_MAX_SIZE];  /**< Boot keyboard output report characteristics value*/
    uint8_t
    mouse_input_report_val[HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE];  /**< Boot mouse input report characteristics value*/
};


/*
 * LOCAL FUNCTION DECLARATIONS
 *****************************************************************************************
 */
static sdk_err_t hids_init(void);
static void      hids_write_att_cb(uint8_t conn_idx, const gatts_write_req_cb_t *p_param);
static void      hids_read_att_cb(uint8_t conn_idx, const gatts_read_req_cb_t *p_param);
static void      hids_cccd_set_cb(uint8_t conn_idx, uint16_t handle, uint16_t cccd_value);

/*
 * LOCAL VARIABLE DEFINITIONS
 *******************************************************************************
 */
static struct hids_env_t s_hids_env;


/**@brief Full HID Service Database Description - Used to add attributes into the database. */
static const attm_desc_t hids_attr_tab[HIDS_IDX_NB] = {
    // HID Service Declaration
    [HIDS_IDX_SVC] = {BLE_ATT_DECL_PRIMARY_SERVICE, READ_PERM_UNSEC, 0, 0},

    // Protocol Mode Characteristic - Declaration
    [HIDS_IDX_PROTOCOL_MODE_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Protocol Mode Characteristic - Value
    [HIDS_IDX_PROTOCOL_MODE_VAL]  = {
        BLE_ATT_CHAR_PROTOCOL_MODE, READ_PERM(UNAUTH) | WRITE_CMD_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(uint8_t)
    },

    // Input Report1 Characteristic - Declaration
    [HIDS_IDX_INPUT1_REPORT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Input Report1 Characteristic - Value
    [HIDS_IDX_INPUT1_REPORT_VAL]  = {
        BLE_ATT_CHAR_REPORT, READ_PERM(UNAUTH) | NOTIFY_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_REPORT_MAX_SIZE
    },
    // Input Report1 Characteristic - Descriptor: CCCD
    [HIDS_IDX_INPUT1_REPORT_CCCD] = {BLE_ATT_DESC_CLIENT_CHAR_CFG, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH), 0, 0},
    // Input Report1 Characteristic - Descriptor: Report Reference
    [HIDS_IDX_INPUT1_REPORT_REF]  = {
        BLE_ATT_DESC_REPORT_REF, READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(hids_report_ref_t)
    },

    // Input Report2 Characteristic - Declaration
    [HIDS_IDX_INPUT2_REPORT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Input Report2 Characteristic - Value
    [HIDS_IDX_INPUT2_REPORT_VAL]  = {
        BLE_ATT_CHAR_REPORT, READ_PERM(UNAUTH) | NOTIFY_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_REPORT_MAX_SIZE
    },
    // Input Report2 Characteristic - Descriptor: CCCD
    [HIDS_IDX_INPUT2_REPORT_CCCD] = {BLE_ATT_DESC_CLIENT_CHAR_CFG, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH), 0, 0},
    // Input Report2 Characteristic - Descriptor: Report Reference
    [HIDS_IDX_INPUT2_REPORT_REF]  = {
        BLE_ATT_DESC_REPORT_REF, READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(hids_report_ref_t)
    },

    // Input Report3 Characteristic - Declaration
    [HIDS_IDX_INPUT3_REPORT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Input Report3 Characteristic - Value
    [HIDS_IDX_INPUT3_REPORT_VAL]  = {
        BLE_ATT_CHAR_REPORT, READ_PERM(UNAUTH) | NOTIFY_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_REPORT_MAX_SIZE
    },
    // Input Report3 Characteristic - Descriptor: CCCD
    [HIDS_IDX_INPUT3_REPORT_CCCD] = {BLE_ATT_DESC_CLIENT_CHAR_CFG, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH), 0, 0},
    // Input Report3 Characteristic - Descriptor: Report Reference
    [HIDS_IDX_INPUT3_REPORT_REF]  = {
        BLE_ATT_DESC_REPORT_REF, READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(hids_report_ref_t)
    },

    // Output Report Characteristic - Declaration
    [HIDS_IDX_OUTPUT_REPORT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Output Report Characteristic - Value
    [HIDS_IDX_OUTPUT_REPORT_VAL]  = {
        BLE_ATT_CHAR_REPORT, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH) | WRITE_CMD_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_REPORT_MAX_SIZE
    },
    // Output Report Characteristic - Descriptor: Report Reference
    [HIDS_IDX_OUTPUT_REPORT_REF]  = {
        BLE_ATT_DESC_REPORT_REF, READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(hids_report_ref_t)
    },

    // Feature Report Characteristic - Declaration
    [HIDS_IDX_FEATURE_REPORT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Feature Report Characteristic - Value
    [HIDS_IDX_FEATURE_REPORT_VAL]  = {
        BLE_ATT_CHAR_REPORT, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_REPORT_MAX_SIZE
    },
    // Feature Report Characteristic - Descriptor: Report Reference
    [HIDS_IDX_FEATURE_REPORT_REF]  = {
        BLE_ATT_DESC_REPORT_REF, READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(hids_report_ref_t)
    },

    // Report Map Characteristic - Declaration
    [HIDS_IDX_REPORT_MAP_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Report Map Characteristic - Value
    [HIDS_IDX_REPORT_MAP_VAL]  = {
        BLE_ATT_CHAR_REPORT_MAP,     READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_REPORT_MAP_MAX_SIZE
    },

    // Boot Keyboard Input Report Characteristic - Declaration
    [HIDS_IDX_BOOT_KB_IN_RPT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Boot Keyboard Input Report Characteristic - Value
    [HIDS_IDX_BOOT_KB_IN_RPT_VAL]  = {
        BLE_ATT_CHAR_BOOT_KB_IN_REPORT,
        READ_PERM(UNAUTH) | NOTIFY_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_BOOT_KB_IN_REPORT_MAX_SIZE
    },
    // Boot Keyboard Input Report Characteristic - Descriptor: CCCD
    [HIDS_IDX_BOOT_KB_IN_RPT_CCCD] = {BLE_ATT_DESC_CLIENT_CHAR_CFG, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH), 0, 0},

    // Boot Keyboard Output Report Characteristic - Declaration
    [HIDS_IDX_BOOT_KB_OUT_RPT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Boot Keyboard Output Report Characteristic - Value
    [HIDS_IDX_BOOT_KB_OUT_RPT_VAL]  = {
        BLE_ATT_CHAR_BOOT_KB_OUT_REPORT,
        READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH) | WRITE_CMD_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_BOOT_KB_OUT_REPORT_MAX_SIZE
    },

    // Boot Mouse Input Report Characteristic - Declaration
    [HIDS_IDX_BOOT_MS_IN_RPT_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // Boot Mouse Input Report Characteristic - Value
    [HIDS_IDX_BOOT_MS_IN_RPT_VAL]  = {
        BLE_ATT_CHAR_BOOT_MOUSE_IN_REPORT,
        READ_PERM(UNAUTH) | NOTIFY_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE
    },
    // Boot Mouse Input Report Characteristic - Descriptor: CCCD
    [HIDS_IDX_BOOT_MS_IN_RPT_CCCD] = {BLE_ATT_DESC_CLIENT_CHAR_CFG, READ_PERM(UNAUTH) | WRITE_REQ_PERM(UNAUTH), 0, 0},

    // HID Information Characteristic - Declaration
    [HIDS_IDX_HID_INFO_CHAR] = {BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC, 0, 0},
    // HID Information Characteristic - Value
    [HIDS_IDX_HID_INFO_VAL]  = {
        BLE_ATT_CHAR_HID_INFO,       READ_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(hids_hid_info_t)
    },

    // HID Control Point Characteristic - Declaration
    [HIDS_IDX_CTRL_POINT_CHAR] = {
        BLE_ATT_DECL_CHARACTERISTIC, READ_PERM_UNSEC,
        0, 0
    },
    // HID Control Point Characteristic - Value
    [HIDS_IDX_CTRL_POINT_VAL]  = {
        BLE_ATT_CHAR_HID_CTNL_PT,    WRITE_CMD_PERM(UNAUTH),
        ATT_VAL_LOC_USER, sizeof(uint8_t)
    },
};

/**@brief HIDS interface required by profile manager. */
static ble_prf_manager_cbs_t hids_mgr_cbs = {
    (prf_init_func_t)hids_init,
    NULL,
    NULL
};

/**@brief HIDS GATT server Callbacks. */
static gatts_prf_cbs_t hids_gatts_cbs = {
    hids_read_att_cb,
    hids_write_att_cb,
    NULL,
    NULL,
    hids_cccd_set_cb
};

/**@brief HIDS Information. */
static const prf_server_info_t hids_prf_info = {
    .max_connection_nb = HIDS_CONNECTION_MAX,
    .manager_cbs       = &hids_mgr_cbs,
    .gatts_prf_cbs     = &hids_gatts_cbs
};

/*
 * LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 */

/**
 *****************************************************************************************
 * @brief Initialize HID service and create db in att.
 *
 * @return BLE_ATT_ERR_NO_ERROR on success, otherwise error code.
 *****************************************************************************************
 */
static sdk_err_t hids_init(void)
{
    const uint8_t     hids_svc_uuid[] = BLE_ATT_16_TO_16_ARRAY(BLE_ATT_SVC_HID);
    gatts_create_db_t gatts_db;
    uint16_t          start_hdl       = PRF_INVALID_HANDLE; /* The start hanlde is an in/out
                                                             * parameter of ble_gatts_srvc_db_create().
                                                             * It must be set with PRF_INVALID_HANDLE
                                                             * to be allocated automatically by BLE Stack. */
    sdk_err_t ret;
    ret = memset_s(&gatts_db, sizeof(gatts_db), 0, sizeof(gatts_db));
    if (ret < 0) {
        return ret;
    }

    gatts_db.shdl                 = &start_hdl;
    gatts_db.uuid                 = (uint8_t*)hids_svc_uuid;
    gatts_db.attr_tab_cfg         = (uint8_t*)&s_hids_env.char_mask;
    gatts_db.max_nb_attr          = HIDS_IDX_NB;
    gatts_db.srvc_perm            = 0;
    gatts_db.attr_tab_type        = SERVICE_TABLE_TYPE_16;
    gatts_db.attr_tab.attr_tab_16 = hids_attr_tab;

    sdk_err_t   status = ble_gatts_srvc_db_create(&gatts_db);
    if (SDK_SUCCESS == status) {
        s_hids_env.start_hdl = *gatts_db.shdl;
    }
    return status;
}

/**
 *****************************************************************************************
 * @brief Handles reception of the read request.
 *
 * @param[in] conn_idx: Connection index.
 * @param[in] p_param:  Pointer to the parameters of the read request.
 *****************************************************************************************
 */
static void hids_read_att_cb(uint8_t conn_idx, const gatts_read_req_cb_t *p_param)
{
    uint8_t handle = p_param->handle;
    uint8_t tab_index = prf_find_idx_by_handle(handle, s_hids_env.start_hdl,
                        HIDS_IDX_NB,
                        (uint8_t *)&s_hids_env.char_mask);
    gatts_read_cfm_t cfm;

    cfm.handle = handle;
    cfm.status = BLE_SUCCESS;

    switch (tab_index) {
        case HIDS_IDX_PROTOCOL_MODE_VAL:
            cfm.length = sizeof(uint8_t);
            cfm.value  = (uint8_t *)(&s_hids_env.protocol_mode);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_INPUT1_REPORT_VAL:
            cfm.length = s_hids_env.hids_init.input_report_array[INDEX_0].value_len;
            cfm.value  = (uint8_t *)(&s_hids_env.input_report_val[INDEX_0]);
            break;

        case HIDS_IDX_INPUT1_REPORT_CCCD:
            cfm.length = sizeof(uint16_t);
            cfm.value  = (uint8_t *)(&s_hids_env.input_cccd[INDEX_0][conn_idx]);
            break;

        case HIDS_IDX_INPUT1_REPORT_REF:
            cfm.length = sizeof(hids_report_ref_t);
            cfm.value  = (uint8_t *)(&s_hids_env.hids_init.input_report_array[INDEX_0].ref);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_INPUT2_REPORT_VAL:
            cfm.length = s_hids_env.hids_init.input_report_array[INDEX_1].value_len;
            cfm.value  = (uint8_t *)(&s_hids_env.input_report_val[INDEX_1]);
            break;

        case HIDS_IDX_INPUT2_REPORT_CCCD:
            cfm.length = sizeof(uint16_t);
            cfm.value  = (uint8_t *)(&s_hids_env.input_cccd[INDEX_1][conn_idx]);
            break;

        case HIDS_IDX_INPUT2_REPORT_REF:
            cfm.length = sizeof(hids_report_ref_t);
            cfm.value  = (uint8_t *)(&s_hids_env.hids_init.input_report_array[INDEX_1].ref);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_INPUT3_REPORT_VAL:
            cfm.length = s_hids_env.hids_init.input_report_array[INDEX_2].value_len;
            cfm.value  = (uint8_t *)(&s_hids_env.input_report_val[INDEX_2]);
            break;

        case HIDS_IDX_INPUT3_REPORT_CCCD:
            cfm.length = sizeof(uint16_t);
            cfm.value  = (uint8_t *)(&s_hids_env.input_cccd[INDEX_2][conn_idx]);
            break;

        case HIDS_IDX_INPUT3_REPORT_REF:
            cfm.length = sizeof(hids_report_ref_t);
            cfm.value  = (uint8_t *)(&s_hids_env.hids_init.input_report_array[INDEX_2].ref);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_OUTPUT_REPORT_VAL:
            cfm.length = s_hids_env.hids_init.output_report.value_len;
            cfm.value  = (uint8_t *)(&s_hids_env.output_report_val);
            break;

        case HIDS_IDX_OUTPUT_REPORT_REF:
            cfm.length = sizeof(hids_report_ref_t);
            cfm.value  = (uint8_t *)(&s_hids_env.hids_init.output_report.ref);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_FEATURE_REPORT_VAL:
            cfm.length = s_hids_env.hids_init.feature_report.value_len;
            cfm.value  = (uint8_t *)(&s_hids_env.feature_report_val);
            break;

        case HIDS_IDX_FEATURE_REPORT_REF:
            cfm.length = sizeof(hids_report_ref_t);
            cfm.value  = (uint8_t *)(&s_hids_env.hids_init.feature_report.ref);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_REPORT_MAP_VAL:
            cfm.length = s_hids_env.hids_init.report_map.len;
            cfm.value  = (uint8_t *)(s_hids_env.hids_init.report_map.p_map);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_BOOT_KB_IN_RPT_VAL:
            cfm.length = HIDS_BOOT_KB_IN_REPORT_MAX_SIZE;
            cfm.value  = (uint8_t *)(&s_hids_env.kb_input_report_val);
            break;

        case HIDS_IDX_BOOT_KB_IN_RPT_CCCD:
            cfm.length = sizeof(uint16_t);
            cfm.value  = (uint8_t *)(&s_hids_env.kb_input_cccd[conn_idx]);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_BOOT_KB_OUT_RPT_VAL:
            cfm.length = HIDS_IDX_BOOT_KB_IN_RPT_VAL;
            cfm.value  = (uint8_t *)(&s_hids_env.kb_output_report_val);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_BOOT_MS_IN_RPT_VAL:
            cfm.length = HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE;
            cfm.value  = (uint8_t *)(&s_hids_env.mouse_input_report_val);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_BOOT_MS_IN_RPT_CCCD:
            cfm.length = sizeof(uint16_t);
            cfm.value  = (uint8_t *)&s_hids_env.mouse_input_cccd[conn_idx];
            break;
        /*----------------------------------------------------------------------------------*/

        case HIDS_IDX_HID_INFO_VAL:
            cfm.length = sizeof(hids_hid_info_t);
            cfm.value  = (uint8_t *)(&s_hids_env.hids_init.hid_info);
            break;
        /*----------------------------------------------------------------------------------*/
        case HIDS_IDX_CTRL_POINT_VAL:
            cfm.length = sizeof(uint16_t);
            cfm.value  = (uint8_t *)(&s_hids_env.ctrl_pt);
            break;
        /*----------------------------------------------------------------------------------*/
        default:
            cfm.length = 0;
            cfm.status = BLE_ATT_ERR_INVALID_HANDLE;
            break;
    }
    ble_gatts_read_cfm(conn_idx, &cfm);
}

/**
 *****************************************************************************************
 * @brief Check Input Report Characteristic cccd value.
 *
 * @param[in] p_evt: Pointer of HID Service event.
 * @param[in] cccd_value: Cccd value.
 *****************************************************************************************
 */
static void hids_cccd_check(hids_evt_t *p_evt, uint16_t cccd_value)
{
    p_evt->evt_type = ((cccd_value == PRF_CLI_START_NTF) ?
                       HIDS_EVT_IN_REP_NOTIFY_ENABLED :
                       HIDS_EVT_IN_REP_NOTIFY_DISABLED);
}

/**
 *****************************************************************************************
 * @brief Function for handling write events to a cccd value.
 *
 * @param[in]   tab_index      CCCD index in DB.
 * @param[in]   conn_idx       Connect index.
 * @param[in]   p_evt          Pointer of HID Service event.
 * @param[in]   cccd_value     CCCD Value.
 *****************************************************************************************
 */
static void hids_on_cccd_write(uint8_t tab_index, uint8_t conn_idx, hids_evt_t *p_evt, uint16_t cccd_value)
{
    switch (tab_index) {
        case HIDS_IDX_INPUT1_REPORT_CCCD:
            s_hids_env.input_cccd[INDEX_0][conn_idx] = cccd_value;
            p_evt->report_type = HIDS_REPORT_TYPE_IN1;
            hids_cccd_check(p_evt, cccd_value);
            break;

        case HIDS_IDX_INPUT2_REPORT_CCCD:
            s_hids_env.input_cccd[INDEX_1][conn_idx] = cccd_value;
            p_evt->report_type = HIDS_REPORT_TYPE_IN2;
            hids_cccd_check(p_evt, cccd_value);
            break;

        case HIDS_IDX_INPUT3_REPORT_CCCD:
            s_hids_env.input_cccd[INDEX_2][conn_idx] = cccd_value;
            p_evt->report_type = HIDS_REPORT_TYPE_IN3;
            hids_cccd_check(p_evt, cccd_value);
            break;

        case HIDS_IDX_BOOT_KB_IN_RPT_CCCD:
            s_hids_env.kb_input_cccd[conn_idx] = cccd_value;
            p_evt->report_type = HIDS_REPORT_TYPE_KB_IN;
            hids_cccd_check(p_evt, cccd_value);
            break;

        case HIDS_IDX_BOOT_MS_IN_RPT_CCCD:
            s_hids_env.mouse_input_cccd[conn_idx] = cccd_value;
            p_evt->report_type = HIDS_REPORT_TYPE_MOUSE_IN;
            hids_cccd_check(p_evt, cccd_value);
            break;

        default:
            break;
    }
}

/**
 *****************************************************************************************
 * @brief Function for handling write events to the Protocol Mode value.
 *
 * @param[in]   p_evt       Pointer of HID Service event.
 * @param[in]   p_param     Pointer to the parameters of the write request.
 *****************************************************************************************
 */
static void hids_on_protocol_mode_write(hids_evt_t *p_evt, const gatts_write_req_cb_t *p_param)
{
    if (p_param->length == 1) {
        switch (p_param->value[INDEX_0]) {
            case PROTOCOL_MODE_BOOT:
                p_evt->evt_type = HIDS_EVT_BOOT_MODE_ENTERED;
                break;

            case PROTOCOL_MODE_REPORT:
                p_evt->evt_type = HIDS_EVT_REPORT_MODE_ENTERED;
                break;

            default:
                break;
        }
        s_hids_env.protocol_mode = p_param->value[INDEX_0];
    }
}

/**
 *****************************************************************************************
 * @brief Function for handling write events to the HID Control Point value..
 *
 * @param[in]   p_evt       Pointer of HID Service event.
 * @param[in]   p_param     Pointer to the parameters of the write request.
 *****************************************************************************************
 */
static void hids_on_control_point_write(hids_evt_t *p_evt, const gatts_write_req_cb_t *p_param)
{
    if (p_param->length == 1) {
        switch (p_param->value[INDEX_0]) {
            case HIDS_CONTROL_POINT_SUSPEND:
                p_evt->evt_type = HIDS_EVT_HOST_SUSP;
                break;

            case HIDS_CONTROL_POINT_EXIT_SUSPEND:
                p_evt->evt_type = HIDS_EVT_HOST_EXIT_SUSP;
                break;

            default:
                break;
        }
        s_hids_env.ctrl_pt = p_param->value[INDEX_0];
    }
}


/**
 *****************************************************************************************
 * @brief Handles reception of the write request.
 *
 * @param[in] conn_idx: Connection index.
 * @param[in] p_param:  Pointer to the parameters of the write request.
 *****************************************************************************************
 */
static void hids_write_att_cb(uint8_t conn_idx,
                              const gatts_write_req_cb_t *p_param)
{
    uint16_t handle = p_param->handle;
    uint8_t tab_index = prf_find_idx_by_handle(handle,
                        s_hids_env.start_hdl,
                        HIDS_IDX_NB,
                        (uint8_t *)&s_hids_env.char_mask);
    uint16_t          cccd_value;
    gatts_write_cfm_t cfm;
    hids_evt_t         evt;
    uint8_t ret;

    evt.evt_type = HIDS_EVT_INVALID;
    evt.offset = p_param->offset;
    evt.len = p_param->length;
    evt.data = p_param->value;
    cfm.handle = handle;
    cfm.status = BLE_SUCCESS;

    switch (tab_index) {
        case HIDS_IDX_PROTOCOL_MODE_VAL:
            hids_on_protocol_mode_write(&evt, p_param);
            break;

        case HIDS_IDX_INPUT1_REPORT_CCCD:
        case HIDS_IDX_INPUT2_REPORT_CCCD:
        case HIDS_IDX_INPUT3_REPORT_CCCD:
        case HIDS_IDX_BOOT_KB_IN_RPT_CCCD:
        case HIDS_IDX_BOOT_MS_IN_RPT_CCCD:
            cccd_value = le16toh(&p_param->value[INDEX_0]);
            hids_on_cccd_write(tab_index, conn_idx, &evt, cccd_value);
            break;

        case HIDS_IDX_INPUT1_REPORT_VAL:
            if ((p_param->offset + p_param->length) <= s_hids_env.hids_init.input_report_array[INDEX_0].value_len) {
                ret = memcp_s(&s_hids_env.input_report_val[INDEX_0][p_param->offset], p_param->length,
                              p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_IN1;
            }
            break;

        case HIDS_IDX_INPUT2_REPORT_VAL:
            if ((p_param->offset + p_param->length) <= s_hids_env.hids_init.input_report_array[INDEX_1].value_len) {
                ret = memcpy_s(&s_hids_env.input_report_val[INDEX_1][p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_IN2;
            }
            break;

        case HIDS_IDX_INPUT3_REPORT_VAL:
            if ((p_param->offset + p_param->length) <= s_hids_env.hids_init.input_report_array[INDEX_2].value_len) {
                ret = memcpy_s(&s_hids_env.input_report_val[INDEX_2][p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_IN3;
            }
            break;

        case HIDS_IDX_OUTPUT_REPORT_VAL:
            if ((p_param->offset + p_param->length) <= s_hids_env.hids_init.output_report.value_len) {
                ret = memcpy_s(&s_hids_env.output_report_val[p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_OUT;
            }
            break;

        case HIDS_IDX_FEATURE_REPORT_VAL:
            if ((p_param->offset + p_param->length) <= s_hids_env.hids_init.feature_report.value_len) {
                ret = memcpy_s(&s_hids_env.feature_report_val[p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_FEATURE;
            }
            break;

        case HIDS_IDX_BOOT_KB_IN_RPT_VAL:
            if ((p_param->offset + p_param->length) <= HIDS_BOOT_KB_IN_REPORT_MAX_SIZE) {
                ret = memcpy_s(&s_hids_env.kb_input_report_val[p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_KB_IN;
            }
            break;
        case HIDS_IDX_BOOT_KB_OUT_RPT_VAL:
            if ((p_param->offset + p_param->length) <= HIDS_BOOT_KB_OUT_REPORT_MAX_SIZE) {
                ret = memcpy_s(&s_hids_env.kb_output_report_val[p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_KB_OUT;
            }
            break;
        case HIDS_IDX_BOOT_MS_IN_RPT_VAL:
            if ((p_param->offset + p_param->length) <= HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE) {
                ret = memcpy_s(&s_hids_env.mouse_input_report_val[p_param->offset], p_param->length,
                               p_param->value, p_param->length);
                if (ret < 0) {
                    return ret;
                }
                evt.evt_type = HIDS_EVT_REP_CHAR_WRITE;
                evt.report_type = HIDS_REPORT_TYPE_MOUSE_IN;
            }
            break;

        case HIDS_IDX_CTRL_POINT_VAL:
            hids_on_control_point_write(&evt, p_param);
            break;

        default:
            cfm.status = BLE_ATT_ERR_INVALID_HANDLE;
            break;
    }

    if (BLE_ATT_ERR_INVALID_HANDLE != cfm.status && \
            HIDS_EVT_INVALID != evt.evt_type && \
            s_hids_env.hids_init.evt_handler) {
        evt.conn_idx = conn_idx;
        s_hids_env.hids_init.evt_handler(&evt);
    }

    ble_gatts_write_cfm(conn_idx, &cfm);
}


/**
 *****************************************************************************************
 * @brief Handles reception of the cccd recover request.
 *
 * @param[in]: conn_idx:   Connection index
 * @param[in]: handle:     The handle of cccd attribute.
 * @param[in]: cccd_value: The value of cccd attribute.
 *****************************************************************************************
 */
static void hids_cccd_set_cb(uint8_t conn_idx, uint16_t handle, uint16_t cccd_value)
{
    hids_evt_t  evt;
    evt.evt_type = HIDS_EVT_INVALID;
    if (!prf_is_cccd_value_valid(cccd_value)) {
        return;
    }

    uint8_t   tab_index = prf_find_idx_by_handle(handle,
                          s_hids_env.start_hdl,
                          HIDS_IDX_NB,
                          (uint8_t *)&s_hids_env.char_mask);
    hids_on_cccd_write(tab_index, conn_idx, &evt, cccd_value);
    if (HIDS_EVT_INVALID != evt.evt_type && \
            s_hids_env.hids_init.evt_handler) {
        evt.conn_idx = conn_idx;
        s_hids_env.hids_init.evt_handler(&evt);
    }
}

/**
 *****************************************************************************************
 * @brief HIDS Characteristic mask init.
 *
 * @param[in]: p_hids_init:   Pointer to the hids init.
 *****************************************************************************************
 */
static void hids_char_mask_init(hids_init_t *p_hids_init)
{
    s_hids_env.char_mask[INDEX_0] = 0x01;
    s_hids_env.char_mask[INDEX_1] = 0x00;
    s_hids_env.char_mask[INDEX_2] = 0x60;
    s_hids_env.char_mask[INDEX_3] = 0x80;
    s_hids_env.char_mask[INDEX_4] = 0x07;
    if (p_hids_init->is_kb || p_hids_init->is_mouse) {
        s_hids_env.char_mask[INDEX_0] |= 0x06;
        if (p_hids_init->is_kb) {
            s_hids_env.char_mask[INDEX_2] |= 0x80;
            s_hids_env.char_mask[INDEX_3] |= 0x0f;
        }
        if (p_hids_init->is_mouse) {
            s_hids_env.char_mask[INDEX_3] |= 0x70;
        }
    }
    switch (p_hids_init->input_report_count) {
        case REPORT_CNT_0: // do noting
            break;
        case REPORT_CNT_1:
            s_hids_env.char_mask[INDEX_0] |= 0x78;
            break;
        case REPORT_CNT_2:
            s_hids_env.char_mask[INDEX_0] |= 0xf8;
            s_hids_env.char_mask[INDEX_1] |= 0x07;
            break;
        default: // max count is 3
            s_hids_env.char_mask[INDEX_0] |= 0xf8;
            s_hids_env.char_mask[INDEX_1] |= 0x7f;
            break;
    }

    if (p_hids_init->out_report_sup) {
        s_hids_env.char_mask[INDEX_1] |= 0x80;
        s_hids_env.char_mask[INDEX_2] |= 0x03;
    }

    if (p_hids_init->feature_report_sup) {
        s_hids_env.char_mask[INDEX_2] |= 0x1c;
    }
}

/**
 *****************************************************************************************
 * @brief Send an input report.
 *
 * @param[in] conn_idx: Connection index.
 * @param[in] char_idx: Input report Characteristic inedx.
 * @param[in] p_data: Pointer to data to be sent.
 * @param[in] length: Length of data to be sent.
 *
 * @return BLE_SDK_SUCCESS on success, otherwise an error code.
 *****************************************************************************************
 */
static sdk_err_t hids_in_rep_notify(uint8_t conn_idx, uint8_t char_idx, uint8_t *p_data, uint16_t length)
{
    sdk_err_t   error_code;
    gatts_noti_ind_t hids_noti;
    hids_noti.type   = BLE_GATT_NOTIFICATION;
    hids_noti.handle = prf_find_handle_by_idx(char_idx,
                                              s_hids_env.start_hdl,
                                              (uint8_t *)&s_hids_env.char_mask);
    hids_noti.length = length;
    hids_noti.value  = p_data;

    error_code = ble_gatts_noti_ind(conn_idx, &hids_noti);
    return error_code;
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 */
sdk_err_t hids_service_init(hids_init_t *p_hids_init)
{
    sdk_err_t ret;
    if (p_hids_init == NULL) {
        return SDK_ERR_POINTER_NULL;
    }
    ret = memcpy_s(&s_hids_env.hids_init, sizeof(hids_init_t), p_hids_init, sizeof(hids_init_t));
    if (ret < 0) {
        return ret;
    }
    hids_char_mask_init(p_hids_init);
    s_hids_env.protocol_mode = DEFAULT_PROTOCOL_MODE;
    s_hids_env.ctrl_pt = INITIAL_VALUE_HID_CONTROL_POINT;
    return ble_server_prf_add(&hids_prf_info);
}


sdk_err_t hids_input_rep_send(uint8_t conn_idx, uint8_t rep_idx, uint8_t *p_data, uint16_t uint16_t length)
{
    uint16_t len = length;
    sdk_err_t ret;
    static const uint8_t char_idx[] = {HIDS_IDX_INPUT1_REPORT_VAL,
                                       HIDS_IDX_INPUT2_REPORT_VAL, HIDS_IDX_INPUT3_REPORT_VAL};
    sdk_err_t   error_code = SDK_ERR_NTF_DISABLED;
    if (rep_idx >= IN_REPORT_MAX_COUNT || p_data == NULL || len == 0) {
        return SDK_ERR_INVALID_PARAM;
    }
    len = ((length > HIDS_REPORT_MAX_SIZE) ? HIDS_REPORT_MAX_SIZE : len);
    ret = memcpy_s(&s_hids_env.input_report_val[rep_idx], len, p_data, len);
    if (ret < 0) {
        return ret;
    }
    if (s_hids_env.input_cccd[rep_idx][conn_idx] == PRF_CLI_START_NTF) {
        error_code = hids_in_rep_notify(conn_idx, char_idx[rep_idx], p_data, len);
    }
    return error_code;
}

sdk_err_t hids_boot_kb_in_rep_send(uint8_t conn_idx, uint8_t *p_data, uint16_t length)
{
    sdk_err_t   error_code = SDK_ERR_NTF_DISABLED;
    uint16_t len = length;
    if (p_data == NULL || len == 0) {
        return SDK_ERR_INVALID_PARAM;
    }
    len = ((len > HIDS_BOOT_KB_IN_REPORT_MAX_SIZE) ? HIDS_BOOT_KB_IN_REPORT_MAX_SIZE : len);
    error_code = memcpy_s(&s_hids_env.kb_input_report_val, len, p_data, len);
    if （error_code < 0) {
        return error_code;
    }
    if (s_hids_env.kb_input_cccd[conn_idx] == PRF_CLI_START_NTF) {
        error_code = hids_in_rep_notify(conn_idx, HIDS_IDX_BOOT_KB_IN_RPT_VAL, p_data, len);
    }
    return error_code;
}

sdk_err_t hids_boot_mouse_in_rep_send(uint8_t conn_idx, uint8_t *p_data, uint16_t length)
{
    sdk_err_t   error_code = SDK_ERR_NTF_DISABLED;
    uint16_t len = length;
    if (p_data == NULL || length < HIDS_BOOT_MOUSE_IN_REPORT_MIN_SIZE) {
        return SDK_ERR_INVALID_PARAM;
    }
    len = ((len > HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE) ? HIDS_BOOT_MOUSE_IN_REPORT_MAX_SIZE : len);
    error_code = memcpy_s(&s_hids_env.mouse_input_report_val, len, p_data, len);
    if （error_code < 0) {
        return error_code;
    }
    if (s_hids_env.mouse_input_cccd[conn_idx] == PRF_CLI_START_NTF) {
        error_code = hids_in_rep_notify(conn_idx, HIDS_IDX_BOOT_MS_IN_RPT_VAL, p_data, len);
    }
    return error_code;
}
