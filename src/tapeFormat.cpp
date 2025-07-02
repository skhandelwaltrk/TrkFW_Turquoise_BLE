#include "tapeFormat.h"
#include "common.h"

static void parseBleAdvData_QuartzTMP117(le_advertising_info *info, BlePacket_QuartzTMP117 *blePkt);
static void parseBleAdvData_QuartzOPT3110(le_advertising_info *info, BlePacket_QuartzOPT3110 *blePkt);
static void parseBleAdvData_QuartzIAT(le_advertising_info *info, BlePacket_IAT *blePkt);
static void parseBleAdvData_QuartzDPD(le_advertising_info *info, BlePacket_DPD *blePkt);
static uint8_t getEvtFlag_QuartzTMP117(le_advertising_info *info);
static uint8_t getEvtFlag_QuartzOPT3110(le_advertising_info *info);
static uint8_t getEvtFlag_QuartzIAT(le_advertising_info *info);
static uint8_t getEvtFlag_QuartzDPD(le_advertising_info *info);
static void removeChar(char *str, char target);

BlePacketType getBlePacketType(le_advertising_info *info) {
    uint16_t tapeId = getTapeId(info);
    switch (tapeId) {
        case QUARTZ_SENSOR_TMP117_TAPE_ID:
            return QuartzSensor_TMP117;
        case QUARTZ_SENSOR_OPT3110_TAPE_ID:
            return QuartzSensor_OPT3110;
        case QUARTZ_SENSOR_IAT_TAPE_ID:
            return QuartzSensor_IAT;
        case QUARTZ_SENSOR_DPD_TAPE_ID:
            return QuartzSensor_DPD;
        default:
            //TRK_PRINTF1("ERROR: Unknown Ble packet type!");
            return QuartzSensor_Unknown;
    }
}

uint8_t getQaurtzEventFlag(le_advertising_info *info) {
    /* Update the BLE event flag for Turquoise GW */
    return ((uint8_t)UpdateEventFlagForQuartz(info->data[QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_EVT_FLAG_OFFSET]));
}

void parseBleDataPacket(le_advertising_info *info, BleDataPacket *bleDataPkt) {
    if (info == nullptr) {
        TRK_PRINTF1("ERROR: nullptr - Could not parse BLE adv data to BLE data packet.");
        exit(EXIT_ERR_NULL_PTR);
    }

    /* Determine the BLE packet type based on the tapeID */
    bleDataPkt->blePktType = getBlePacketType(info);

    switch (bleDataPkt->blePktType) {
        case QuartzSensor_TMP117:
            parseBleAdvData_QuartzTMP117(info, &bleDataPkt->blePktStrct.blePkt_TMP117);
            break;
        case QuartzSensor_OPT3110:
            parseBleAdvData_QuartzOPT3110(info, &bleDataPkt->blePktStrct.blePkt_OPT3110);
            break;
        case QuartzSensor_IAT:
            parseBleAdvData_QuartzIAT(info, &bleDataPkt->blePktStrct.blePkt_IAT);
            break;
        case QuartzSensor_DPD:
            parseBleAdvData_QuartzDPD(info, &bleDataPkt->blePktStrct.blePkt_DPD);
            break;
        default:
            //TRK_PRINTF1("ERROR: Unknown BLE packet type detected");
            break;
    }
}

/* Helper function */
static void removeChar(char *str, char target) {
    char *src = str, *dst = str;

    while (*src) {
        if (*src != target) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

static void parseBleAdvData_QuartzTMP117(le_advertising_info *info, BlePacket_QuartzTMP117 *blePkt) {
    if (info == nullptr || blePkt == nullptr) {
        TRK_PRINTF1("ERROR: nullptr - Could not parse BLE adv data to TMP117 BLE data packet.");
        exit(EXIT_ERR_NULL_PTR);
    }

    /* MAC Address */
    memset(blePkt->mac_addr, 0, sizeof(blePkt->mac_addr));
    ba2str(&info->bdaddr, blePkt->mac_addr);
    removeChar(blePkt->mac_addr,':');

    /* Process and save the BLE info data - 24 bytes */
    blePkt->fid = getFid(info);
    blePkt->evt_flag = getEvtFlag_QuartzTMP117(info);
    blePkt->t0 = getTemperatureData(info, QUARTZ_TMP117_BLE_ADV_PKT_T0_OFFSET);
    blePkt->t0_ts = getTimestampU16(info, QUARTZ_TMP117_BLE_ADV_PKT_T0_TS_OFFSET);
    blePkt->t1 = getTemperatureData(info, QUARTZ_TMP117_BLE_ADV_PKT_T1_OFFSET);
    blePkt->t1_ts = getTimestampU16(info, QUARTZ_TMP117_BLE_ADV_PKT_T1_TS_OFFSET);
    blePkt->t2 = getTemperatureData(info, QUARTZ_TMP117_BLE_ADV_PKT_T2_OFFSET);
    blePkt->t2_ts = getTimestampU16(info, QUARTZ_TMP117_BLE_ADV_PKT_T2_TS_OFFSET);
    blePkt->pid = getLimePid(info, QUARTZ_TMP117_BLE_ADV_PKT_LIME_PID_OFFSET);
    blePkt->lime_bat = getLimeBat(info);
    blePkt->seqId = getRecordSequence(info, QUARTZ_TMP117_BLE_ADV_PKT_REC_SEQ_OFFSET);
    blePkt->tapeId = getTapeId(info);
    blePkt->bat = getBatVoltage(info);
    blePkt->rssi = getTapeRssi(info);
}

static void parseBleAdvData_QuartzOPT3110(le_advertising_info *info, BlePacket_QuartzOPT3110 *blePkt) {
    if (info == nullptr || blePkt == nullptr) {
        TRK_PRINTF1("ERROR: nullptr - Could not parse BLE adv data to OPT3110 BLE data packet.");
        exit(EXIT_ERR_NULL_PTR);
    }

    TRK_PRINTF1("Parsing OPT3110 BLE Adv Data ...");

    /* MAC Address */
    memset(blePkt->mac_addr, 0, sizeof(blePkt->mac_addr));
    ba2str(&info->bdaddr, blePkt->mac_addr);
    removeChar(blePkt->mac_addr,':');

    /* Process and save the BLE info data - 24 bytes */
    blePkt->fid = getFid(info);
    blePkt->evt_flag = getEvtFlag_QuartzOPT3110(info);
    blePkt->t0 = getTemperatureData(info, QUARTZ_OPT3110_BLE_ADV_PKT_T0_OFFSET);
    blePkt->t0_ts = getTimestampU16(info, QUARTZ_OPT3110_BLE_ADV_PKT_T0_TS_OFFSET);
    blePkt->l0 = getTemperatureData(info, QUARTZ_OPT3110_BLE_ADV_PKT_L0_OFFSET);
    blePkt->l0_ts = getTimestampU16(info, QUARTZ_OPT3110_BLE_ADV_PKT_L0_TS_OFFSET);
    blePkt->l1 = getTemperatureData(info, QUARTZ_OPT3110_BLE_ADV_PKT_L1_OFFSET);
    blePkt->l1_ts = getTimestampU16(info, QUARTZ_OPT3110_BLE_ADV_PKT_L1_TS_OFFSET);
    blePkt->pid = getLimePid(info, QUARTZ_OPT3110_BLE_ADV_PKT_LIME_PID_OFFSET);
    blePkt->lime_bat = getLimeBat(info);
    blePkt->seqId = getRecordSequence(info, QUARTZ_OPT3110_BLE_ADV_PKT_REC_SEQ_OFFSET);
    blePkt->tapeId = getTapeId(info);
    blePkt->bat = getBatVoltage(info);
    blePkt->rssi = getTapeRssi(info);
}

static void parseBleAdvData_QuartzIAT(le_advertising_info *info, BlePacket_IAT *blePkt) {
    if (info == nullptr || blePkt == nullptr) {
        TRK_PRINTF1("ERROR: nullptr - Could not parse BLE adv data to OPT3110 BLE data packet.");
        exit(EXIT_ERR_NULL_PTR);
    }

    /* MAC Address */
    memset(blePkt->mac_addr, 0, sizeof(blePkt->mac_addr));
    ba2str(&info->bdaddr, blePkt->mac_addr);
    removeChar(blePkt->mac_addr,':');

    /* Process and save the BLE info data - 24 bytes */
    blePkt->fid = getFid(info);
    blePkt->evt_flag = getEvtFlag_QuartzIAT(info);
    blePkt->t0 = (float)info->data[QUARTZ_IAT_BLE_ADV_PKT_T0_OFFSET];
    blePkt->t1 = (float)info->data[QUARTZ_IAT_BLE_ADV_PKT_T1_OFFSET];
    blePkt->t1_ts = getTimestampU32(info, QUARTZ_IAT_BLE_ADV_PKT_T1_TS_OFFSET);
    blePkt->l0 = getLightData(info, QUARTZ_IAT_BLE_ADV_PKT_L0_OFFSET);
    blePkt->l0_ts = getTimestampU32(info, QUARTZ_IAT_BLE_ADV_PKT_L0_TS_OFFSET);
    blePkt->a0_val = (int8_t)info->data[QUARTZ_IAT_BLE_ADV_PKT_A0_VAL_OFFSET];
    blePkt->a0_count = (uint8_t)info->data[QUARTZ_IAT_BLE_ADV_PKT_A0_CNT_OFFSET];
    blePkt->ts = getTimestampU32(info, QUARTZ_IAT_BLE_ADV_PKT_TS_OFFSET);
    blePkt->tapeId = getTapeId(info);
    blePkt->bat = getBatVoltage(info);
    blePkt->rssi = getTapeRssi(info);    
}

static void parseBleAdvData_QuartzDPD(le_advertising_info *info, BlePacket_DPD *blePkt) {
    if (info == nullptr || blePkt == nullptr) {
        TRK_PRINTF1("ERROR: nullptr - Could not parse BLE adv data to OPT3110 BLE data packet.");
        exit(EXIT_ERR_NULL_PTR);
    }

    /* MAC Address */
    memset(blePkt->mac_addr, 0, sizeof(blePkt->mac_addr));
    ba2str(&info->bdaddr, blePkt->mac_addr);
    removeChar(blePkt->mac_addr,':');

    /* Process and save the BLE info data - 24 bytes */
    blePkt->fid = getFid(info);
    blePkt->evt_flag = getEvtFlag_QuartzDPD(info);
    blePkt->t0 = (float)info->data[QUARTZ_DPD_BLE_ADV_PKT_T0_OFFSET];
    blePkt->t1 = (float)info->data[QUARTZ_DPD_BLE_ADV_PKT_T1_OFFSET];
    blePkt->t1_ts = getTimestampU32(info, QUARTZ_DPD_BLE_ADV_PKT_T1_TS_OFFSET);
    blePkt->l0 = getLightData(info, QUARTZ_DPD_BLE_ADV_PKT_L0_OFFSET);
    blePkt->l0_ts = getTimestampU32(info, QUARTZ_DPD_BLE_ADV_PKT_L0_TS_OFFSET);
    blePkt->l1 = getLightData(info, QUARTZ_DPD_BLE_ADV_PKT_L1_OFFSET);
    blePkt->ts = getTimestampU32(info, QUARTZ_DPD_BLE_ADV_PKT_TS_OFFSET);
    blePkt->tapeId = getTapeId(info);
    blePkt->bat = getBatVoltage(info);
    blePkt->rssi = getTapeRssi(info);    
}

static uint8_t getEvtFlag_QuartzTMP117(le_advertising_info *info) {
    if (info == nullptr) {
        TRK_PRINTF1("ERROR - Null ptr. Cannot get TMP117 BLE Evt Flag");
        return 0;
    }
    uint8_t blePktEvtFlag = info->data[QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_EVT_FLAG_OFFSET];
    return (blePktEvtFlag == 0 ? (uint8_t)QuartzTMP117_NormalMode : blePktEvtFlag);
}

static uint8_t getEvtFlag_QuartzOPT3110(le_advertising_info *info) {
    if (info == nullptr) {
        TRK_PRINTF1("ERROR - Null ptr. Cannot get OPT3110 BLE Evt Flag");
        return 0;
    }
    uint8_t blePktEvtFlag = info->data[QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_EVT_FLAG_OFFSET];
    return (blePktEvtFlag == 0 ? (uint8_t)QuartzOPT3110_NormalMode : blePktEvtFlag);
}

static uint8_t getEvtFlag_QuartzIAT(le_advertising_info *info) {
    if (info == nullptr) {
        TRK_PRINTF1("ERROR - Null ptr. Cannot get IAT BLE Evt Flag");
        return 0;
    }
    uint8_t blePktEvtFlag = info->data[QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_EVT_FLAG_OFFSET];
    return (blePktEvtFlag == 0 ? (uint8_t)QuartzIAT_NormalMode : blePktEvtFlag);
}

static uint8_t getEvtFlag_QuartzDPD(le_advertising_info *info) {
    if (info == nullptr) {
        TRK_PRINTF1("ERROR - Null ptr. Cannot get DPD BLE Evt Flag");
        return 0;
    }
    uint8_t blePktEvtFlag = info->data[QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_EVT_FLAG_OFFSET];
    return (blePktEvtFlag == 0 ? (uint8_t)QuartzDPD_NormalMode : blePktEvtFlag);
}

