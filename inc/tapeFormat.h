#ifndef _TAPEFORMAT_H_
#define _TAPEFORMAT_H_

#include <cstdint>
#include <cstring>
#include <cstdio>
extern "C" {
    #include <bluetooth/bluetooth.h>
    #include <bluetooth/hci.h>
    #include <bluetooth/hci_lib.h>
}

/* Defined header files */
#include "common.h"

#define RFID_GW_COMPANY_IDENTIFIER_HIGH_INDEX                 (2u)
#define RFID_GW_COMPANY_IDENTIFIER_LOW_INDEX                  (3u)
#define RFID_GW_TRACK_IDENTIFIER_HIGH_INDEX                   (4u)
#define RFID_GW_TRACK_IDENTIFIER_LOW_INDEX                    (5u)
#define RFID_GW_PRODUCT_ID_HIGH_INDEX                         (12u)
#define RFID_GW_PRODUCT_ID_LOW_INDEX                          (13u)

#define COMPANY_IDENTIFIER_HIGH_INDEX                         (5u)
#define COMPANY_IDENTIFIER_LOW_INDEX                          (6u)
#define TRACK_IDENTIFIER_HIGH_INDEX                           (7u)
#define TRACK_IDENTIFIER_LOW_INDEX                            (8u)

#define NORDIC_IDENTIFIER_HIGH_BYTE                           (0x59)
#define NORDIC_IDENTIFIER_LOW_BYTE                            (0x00)
#define LIME_MILESTONE_HIGH_BYTE                              (0x49)
#define LIME_MILESTONE_LOW_BYTE                               (0x54)
#define WHITETAPE_HIGH_BYTE                                   (0x52)
#define WHITETAPE_LOW_BYTE                                    (0x58)
#define ULD_HIGH_BYTE                                         (0x52)
#define ULD_LOW_BYTE                                          (0x59)
#define RFID_GW_PRODUCT_ID_HIGH_BYTE                          (0xFF)
#define RFID_GW_PRODUCT_ID_LOW_BYTE                           (0xA5)

/* Tape IDs for the Quartz sensor types */
#define QUARTZ_SENSOR_TMP117_TAPE_ID                          (0xFFFC)        
#define QUARTZ_SENSOR_OPT3110_TAPE_ID                         (0xFFFA)
#define QUARTZ_SENSOR_IAT_TAPE_ID                             (0xFFB1)
#define QUARTZ_SENSOR_DPD_TAPE_ID                             (0xFFB0)

#define QUARTZ_BLE_ADV_PKT_DATA_START_IDX                     (7)
#define QUARTZ_BLE_ADV_PKT_TAPE_ID_IDX                        (QUARTZ_BLE_ADV_PKT_DATA_START_IDX + 21)

#define QUARTZ_BLE_ADV_PKT_TAPE_ID_OFFSET                     (21)
#define QUARTZ_BLE_ADV_PKT_FID_OFFSET                         (0)
#define QUARTZ_BLE_ADV_PKT_EVT_FLAG_OFFSET                    (2)
#define QUARTZ_BLE_ADV_PKT_BAT_VOLT_OFFSET                    (23)

#define QUARTZ_TMP117_BLE_ADV_PKT_T0_OFFSET                   (3)
#define QUARTZ_TMP117_BLE_ADV_PKT_T0_TS_OFFSET                (5)
#define QUARTZ_TMP117_BLE_ADV_PKT_T1_OFFSET                   (7)
#define QUARTZ_TMP117_BLE_ADV_PKT_T1_TS_OFFSET                (9)
#define QUARTZ_TMP117_BLE_ADV_PKT_T2_OFFSET                   (11)
#define QUARTZ_TMP117_BLE_ADV_PKT_T2_TS_OFFSET                (13)
#define QUARTZ_TMP117_BLE_ADV_PKT_LIME_PID_OFFSET             (15)
#define QUARTZ_TMP117_BLE_ADV_PKT_LIME_BAT_OFFSET             (18)
#define QUARTZ_TMP117_BLE_ADV_PKT_REC_SEQ_OFFSET              (19)

#define QUARTZ_OPT3110_BLE_ADV_PKT_T0_OFFSET                   (3)
#define QUARTZ_OPT3110_BLE_ADV_PKT_T0_TS_OFFSET                (5)
#define QUARTZ_OPT3110_BLE_ADV_PKT_L0_OFFSET                   (7)
#define QUARTZ_OPT3110_BLE_ADV_PKT_L0_TS_OFFSET                (9)
#define QUARTZ_OPT3110_BLE_ADV_PKT_L1_OFFSET                   (11)
#define QUARTZ_OPT3110_BLE_ADV_PKT_L1_TS_OFFSET                (13)
#define QUARTZ_OPT3110_BLE_ADV_PKT_LIME_PID_OFFSET             (15)
#define QUARTZ_OPT3110_BLE_ADV_PKT_LIME_BAT_OFFSET             (18)
#define QUARTZ_OPT3110_BLE_ADV_PKT_REC_SEQ_OFFSET              (19)

#define QUARTZ_IAT_BLE_ADV_PKT_T0_OFFSET                       (3)
#define QUARTZ_IAT_BLE_ADV_PKT_T1_OFFSET                       (4)
#define QUARTZ_IAT_BLE_ADV_PKT_T1_TS_OFFSET                    (5)
#define QUARTZ_IAT_BLE_ADV_PKT_L0_OFFSET                       (9)
#define QUARTZ_IAT_BLE_ADV_PKT_L0_TS_OFFSET                    (11)
#define QUARTZ_IAT_BLE_ADV_PKT_A0_VAL_OFFSET                   (15)
#define QUARTZ_IAT_BLE_ADV_PKT_A0_CNT_OFFSET                   (16)
#define QUARTZ_IAT_BLE_ADV_PKT_TS_OFFSET                       (17)

#define QUARTZ_DPD_BLE_ADV_PKT_T0_OFFSET                       (3)
#define QUARTZ_DPD_BLE_ADV_PKT_T1_OFFSET                       (4)
#define QUARTZ_DPD_BLE_ADV_PKT_T1_TS_OFFSET                    (5)
#define QUARTZ_DPD_BLE_ADV_PKT_L0_OFFSET                       (9)
#define QUARTZ_DPD_BLE_ADV_PKT_L0_TS_OFFSET                    (11)
#define QUARTZ_DPD_BLE_ADV_PKT_L1_OFFSET                       (15)
#define QUARTZ_DPD_BLE_ADV_PKT_TS_OFFSET                       (17)

#define EVT_QUARTZ_TMP117_NORMAL_MODE                          (0)
#define EVT_WHITE_TAPE_TEMP_HEARTBEAT_MODE                     (50)
#define EVT_WHITE_TAPE_MOTION_MODE                             (1)
#define EVT_WHITE_TAPE_HIBERNATION_MODE                        (18)
#define EVT_WHITE_TAPE_TEMP_VIOLATION_MODE                     (51)
#define EVT_WHITE_TAPE_CONFIG_MODE                             (56)

#define WHITE_TAPE_DATA_PACKET_LEN                             (24u)
#define WHITE_TAPE_BLE_ADV_INFO_LEN                            (31u)
#define WHITE_TAPE_DATA_EVENT_FLAG_OFFSET                      (9u)

/*
    Event Flag Enums for different white tape configurations.
    Event Flag is saved in byte 22 for all white tape configurations.
*/

/* Quartz Sensor TMP117 white tape configuration - Tape ID 0xFFFC */
typedef enum e_QuartzEvt_TMP117 {
  QuartzTMP117_NormalMode                    = 50,
  QuartzTMP117_InMotionMode                  = 1,
  QuartzTMP117_RunningToSleepMode            = 2,
  QuartzTMP117_EnteringAeroplaneMode         = 4,
  QuartzTMP117_InAeroplaneMode               = 5,
  QuartzTMP117_OutOffAeroplaneMode           = 6,
  QuartzTMP117_EnteringWhiteTapeMode         = 7,
  QuartzTMP117_InWhiteTapeMode               = 17,
  QuartzTMP117_OutOffWhiteTapeMode           = 8,
  QuartzTMP117_EnteringHibernationMode       = 9,
  QuartzTMP117_InHibernationMode             = 18,
  QuartzTMP117_OutOffHibernationMode         = 10,
  QuartzTMP117_CutCircuitResetMode           = 11,
  QuartzTMP117_WatchDogResetMode             = 12,
  QuartzTMP117_ButtonPressResetMode          = 13,
  QuartzTMP117_CPULockResetMode              = 14,
  QuartzTMP117_SoftwareInstructionResetMode  = 15,
  QuartzTMP117_OtherResetsMode               = 16,
  QuartzTMP117_EnteringShutdownMode          = 19,
  QuartzTMP117_InShutdownMode                = 20,
  QuartzTMP117_ExitingShutDownMode           = 22,
  QuartzTMP117_HeartbeatMode                 = 21,
  QuartzTMP117_TemperatureHeartbeatMode      = 50,
  QuartzTMP117_TemperatureViolationMode      = 51,
  QuartzTMP117_ConfigMode                    // TBD, not used yet
} e_QuartzEvt_TMP117;

typedef struct BlePacket_QuartzTMP117 {
    char mac_addr[20];      // White Tape MAC Address
    uint16_t fid;           // Byte [0:1]: FID
    uint8_t evt_flag;       // Byte [2]: Event Flag
    float t0;               // Byte [3:4]: Current Temp (3:t0 exponent (int8_t); 4:t0 fraction (uint8_t))
    uint16_t t0_ts;         // Byte [5:6]: Current Timestamp (t0)
    float t1;               // Byte [7:8]: Min Temp (7:t1 exponent (int8_t); 8:t1 fraction (uint8_t))
    uint16_t t1_ts;         // Byte [9:10]: Timestamp t1
    float t2;               // Byte [11:12]: Min Temp (11:t1 exponent (int8_t); 12:t1 fraction (uint8_t))
    uint16_t t2_ts;         // Byte [13:14]: Timestamp t2
    uint32_t pid;           // Byte [15:17]: 4th, 5th and 6th bytes of LIME
    uint8_t lime_bat;       // Byte [18]: Battery of LIME
    uint16_t seqId;         // Byte [19:20]: Record Sequence
    uint16_t tapeId;        // Byte [21:22]: Tape ID (0xFFFC)
    float bat;              // Byte [23]: Battery voltage of white tape
    int8_t rssi;            // Byte [24]: RSSI
} BlePacket_QuartzTMP117;

/* Quartz Sensor OPT3110 white tape configuration - Tape ID 0xFFFA */
typedef enum e_QuartzEvt_OPT3110 {
  QuartzOPT3110_NormalMode                    = 55,
  QuartzOPT3110_InMotionMode                  = 1,
  QuartzOPT3110_RunningToSleepMode            = 2,
  QuartzOPT3110_EnteringAeroplaneMode         = 4,
  QuartzOPT3110_InAeroplaneMode               = 5,
  QuartzOPT3110_OutOffAeroplaneMode           = 6,
  QuartzOPT3110_EnteringWhiteTapeMode         = 7,
  QuartzOPT3110_InWhiteTapeMode               = 17,
  QuartzOPT3110_OutOffWhiteTapeMode           = 8,
  QuartzOPT3110_EnteringHibernationMode       = 9,
  QuartzOPT3110_InHibernationMode             = 18,
  QuartzOPT3110_OutOffHibernationMode         = 10,
  QuartzOPT3110_CutCircuitResetMode           = 11,
  QuartzOPT3110_WatchDogResetMode             = 12,
  QuartzOPT3110_ButtonPressResetMode          = 13,
  QuartzOPT3110_CPULockResetMode              = 14,
  QuartzOPT3110_SoftwareInstructionResetMode  = 15,
  QuartzOPT3110_OtherResetsMode               = 16,
  QuartzOPT3110_EnteringShutdownMode          = 19,
  QuartzOPT3110_InShutdownMode                = 20,
  QuartzOPT3110_ExitingShutDownMode           = 22,
  QuartzOPT3110_HeartbeatMode                 = 21,
  QuartzOPT3110_TemperatureHeartbeatMode      = 50,
  QuartzOPT3110_TemperatureViolationMode      = 36,
  QuartzOPT3110_LightViolationMode            = 37,
  QuartzOPT3110_ConfigMode                    = 200// TBD, not used yet
} e_QuartzEvt_OPT3110;

typedef struct BlePacket_QuartzOPT3110 {
    char mac_addr[20];      // White Tape MAC Address
    uint16_t fid;           // Byte [0:1]: FID
    uint8_t evt_flag;       // Byte [2]: Event Flag
    float t0;               // Byte [3:4]: Current Temp (3:t0 exponent (int8_t); 4:t0 fraction (uint8_t))
    uint16_t t0_ts;         // Byte [5:6]: Current Timestamp (t0)
    uint16_t l0;            // Byte [7:8]: light l0
    uint16_t l0_ts;         // Byte [9:10]: Timestamp t_l0
    uint16_t l1;            // Byte [11:12]: light Max
    uint16_t l1_ts;         // Byte [13:14]: Timestamp t_l1
    uint32_t pid;           // Byte [15:17]: 4th, 5th and 6th bytes of LIME
    uint8_t lime_bat;       // Byte [18]: Battery of LIME
    uint16_t seqId;         // Byte [19:20]: Record Sequence
    uint16_t tapeId;        // Byte [21:22]: Tape ID (0xFFFA)
    float bat;              // Byte [23]: Battery voltage of white tape
    int8_t rssi;            // Byte [24]: RSSI
} BlePacket_QuartzOPT3110;

/* Quartz Sensor IAT white tape configuration - Tape ID 0xFFB1 */
typedef enum e_QuartzEvt_IAT {
  QuartzIAT_NormalMode                    = 55,
  QuartzIAT_InMotionMode                  = 1,
  QuartzIAT_RunningToSleepMode            = 2,
  QuartzIAT_EnteringAeroplaneMode         = 4,
  QuartzIAT_InAeroplaneMode               = 5,
  QuartzIAT_OutOffAeroplaneMode           = 6,
  QuartzIAT_EnteringWhiteTapeMode         = 7,
  QuartzIAT_InWhiteTapeMode               = 17,
  QuartzIAT_OutOffWhiteTapeMode           = 8,
  QuartzIAT_EnteringHibernationMode       = 9,
  QuartzIAT_InHibernationMode             = 18,
  QuartzIAT_OutOffHibernationMode         = 10,
  QuartzIAT_CutCircuitResetMode           = 11,
  QuartzIAT_WatchDogResetMode             = 12,
  QuartzIAT_ButtonPressResetMode          = 13,
  QuartzIAT_CPULockResetMode              = 14,
  QuartzIAT_SoftwareInstructionResetMode  = 15,
  QuartzIAT_OtherResetsMode               = 16,
  QuartzIAT_EnteringShutdownMode          = 19,
  QuartzIAT_InShutdownMode                = 20,
  QuartzIAT_ExitingShutDownMode           = 22,
  QuartzIAT_HeartbeatMode                 = 21,
  QuartzIAT_TemperatureHeartbeatMode      = 50,
  QuartzIAT_TemperatureViolationMode      = 56,
  QuartzIAT_LightViolationMode            = 57,
  QuartzIAT_ShockViolationMode            = 58,
  QuartzIAT_ConfigMode                    = 200// TBD, not used yet
} e_QuartzEvt_IAT;

typedef struct BlePacket_IAT {
    char mac_addr[20];      // White Tape MAC Address
    uint16_t fid;           // Byte [0:1]: FID
    uint8_t evt_flag;       // Byte [2]: Event Flag
    float t0;               // Byte [3]: Current Temp t0
    float t1;               // Byte [4]: Temperature T1 (violation)
    uint32_t t1_ts;         // Byte [5:8]: Current Timestamp (t0)
    uint16_t l0;            // Byte [9:10]: light l0 (Violation)
    uint32_t l0_ts;         // Byte [11:14]: Timestamp t_l0
    int8_t a0_val;          // Byte [15]: Acceleration a0 value
    uint8_t a0_count;       // Byte [16]: Acceleration a0 count
    uint32_t ts;            // Byte [17:20]: Timestamp ts
    uint16_t tapeId;        // Byte [21:22]: Tape ID (0xFFFA)
    float bat;              // Byte [23]: Battery voltage of white tape
    int8_t rssi;            // Byte [24]: RSSI
} BlePacket_IAT;

/* Quartz Sensor DPD white tape configuration - Tape ID 0xFFB0 */
typedef enum e_QuartzEvt_DPD {
  QuartzDPD_NormalMode                    = 55,
  QuartzDPD_InMotionMode                  = 1,
  QuartzDPD_RunningToSleepMode            = 2,
  QuartzDPD_EnteringAeroplaneMode         = 4,
  QuartzDPD_InAeroplaneMode               = 5,
  QuartzDPD_OutOffAeroplaneMode           = 6,
  QuartzDPD_EnteringWhiteTapeMode         = 7,
  QuartzDPD_InWhiteTapeMode               = 17,
  QuartzDPD_OutOffWhiteTapeMode           = 8,
  QuartzDPD_EnteringHibernationMode       = 9,
  QuartzDPD_InHibernationMode             = 18,
  QuartzDPD_OutOffHibernationMode         = 10,
  QuartzDPD_CutCircuitResetMode           = 11,
  QuartzDPD_WatchDogResetMode             = 12,
  QuartzDPD_ButtonPressResetMode          = 13,
  QuartzDPD_CPULockResetMode              = 14,
  QuartzDPD_SoftwareInstructionResetMode  = 15,
  QuartzDPD_OtherResetsMode               = 16,
  QuartzDPD_EnteringShutdownMode          = 19,
  QuartzDPD_InShutdownMode                = 20,
  QuartzDPD_ExitingShutDownMode           = 22,
  QuartzDPD_HeartbeatMode                 = 21,
  QuartzDPD_TemperatureHeartbeatMode      = 50,
  QuartzDPD_TemperatureViolationMode      = 56,
  QuartzDPD_LightViolationMode            = 57,
  QuartzDPD_ShockViolationMode            = 58,
  QuartzDPD_ConfigMode                    = 200// TBD, not used yet
} e_QuartzEvt_DPD;

typedef struct BlePacket_DPD {
    char mac_addr[20];      // White Tape MAC Address
    uint16_t fid;           // Byte [0:1]: FID
    uint8_t evt_flag;       // Byte [2]: Event Flag
    float t0;               // Byte [3]: Current Temp t0
    float t1;               // Byte [4]: Temperature T1 (violation)
    uint32_t t1_ts;         // Byte [5:8]: Current Timestamp (t0)
    uint16_t l0;            // Byte [9:10]: light l0
    uint32_t l0_ts;         // Byte [11:14]: Timestamp t_l0
    uint16_t l1;            // Byte [15:16]: Current Light l1
    uint16_t ts;            // Byte [17:20]: Timestamp ts
    uint16_t tapeId;        // Byte [21:22]: Tape ID (0xFFFA)
    float bat;              // Byte [23]: Battery voltage of white tape
    int8_t rssi;            // Byte [24]: RSSI
} BlePacket_DPD;

#define QUARTZ_SENSOR_TYPES         (4)
typedef enum BlePacketType {
    QuartzSensor_Unknown,
    QuartzSensor_TMP117,    // TapeId = 0xFFFC
    QuartzSensor_OPT3110,   // TapeId = 0xFFFA
    QuartzSensor_IAT,       // TapedId = 0xFFB1
    QuartzSensor_DPD,       // TapeId = 0xFFB0
    QuartzSensor_Max
} BlePacketType;

typedef union BlePacketEvtStruct {
    e_QuartzEvt_TMP117 e0_TMP117;
    e_QuartzEvt_OPT3110 e0_OPT3110;
    e_QuartzEvt_IAT e0_IAT;
    e_QuartzEvt_DPD e0_DPD;
} BlePacketEvtStruct;

/* 
    Super structure used to get the event flag for different white
    tape configurations.
*/
typedef struct BleEventType {
    BlePacketEvtStruct bleEvt;
    BlePacketType blePktType;
} BleEventType;

typedef union BleDataPacketStruct {
    BlePacket_QuartzTMP117 blePkt_TMP117;
    BlePacket_QuartzOPT3110 blePkt_OPT3110;
    BlePacket_IAT blePkt_IAT;
    BlePacket_DPD blePkt_DPD;
} BleDataPacketStruct;

/* 
    Super structure that is used as queue objects to exchange BLE data
    between the BLE thread and the cloud communication thread.
*/
typedef struct BleDataPacket {
    BleDataPacketStruct blePktStrct;
    BlePacketType blePktType;
} BleDataPacket;

/* Inline Functions */


inline uint16_t getTimestampU16(le_advertising_info *info, uint8_t byteOffset) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + byteOffset;
    return (((uint16_t)info->data[startIdx] << 8) |
            ((uint16_t)info->data[startIdx + 1]));
}

inline uint32_t getTimestampU32(le_advertising_info *info, uint8_t byteOffset) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + byteOffset;
    return ((((uint32_t)info->data[startIdx]) << 24) |
           (((uint32_t)info->data[startIdx + 1]) << 16) |
           (((uint32_t)info->data[startIdx + 2]) << 8)  |
           ((uint32_t)info->data[startIdx + 3]));
}

inline float getTemperatureData(le_advertising_info *info, uint8_t byteOffset) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + byteOffset;
    return ((float)(info->data[startIdx]) + 
            ((float)(info->data[startIdx + 1]))/100.0f);
}

inline uint32_t getLimePid(le_advertising_info *info, uint8_t byteOffset) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + byteOffset;
    return ((((uint32_t)info->data[startIdx + 2]) << 16) |
           (((uint32_t)info->data[startIdx + 1]) << 8)  |
           ((uint32_t)info->data[startIdx]));
}

inline uint16_t getTapeId(le_advertising_info *info) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_TAPE_ID_IDX;
    return (((uint16_t)info->data[startIdx] << 8) |
            ((uint16_t)info->data[startIdx + 1]));
}

inline uint16_t getFid(le_advertising_info *info) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_FID_OFFSET;
    return (((uint16_t)info->data[startIdx] << 8) |
            ((uint16_t)info->data[startIdx + 1]));
}

inline float getBatVoltage(le_advertising_info *info) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_BLE_ADV_PKT_BAT_VOLT_OFFSET;
    return (((float)info->data[startIdx])/10.0f);
}

inline int8_t getTapeRssi(le_advertising_info *info) {
    return ((int8_t)info->data[info->length]);
}

inline uint8_t getLimeBat(le_advertising_info *info) {
    return info->data[QUARTZ_BLE_ADV_PKT_DATA_START_IDX + QUARTZ_OPT3110_BLE_ADV_PKT_LIME_BAT_OFFSET];
}

inline uint16_t getRecordSequence(le_advertising_info *info, uint8_t byteOffset) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + byteOffset;
    return (((uint16_t)info->data[startIdx] << 8) |
            ((uint16_t)info->data[startIdx + 1]));
}

inline uint16_t getLightData(le_advertising_info *info, uint8_t byteOffset) {
    uint8_t startIdx = QUARTZ_BLE_ADV_PKT_DATA_START_IDX + byteOffset;
    return (((uint16_t)info->data[startIdx] << 8) |
            ((uint16_t)info->data[startIdx + 1]));
}

/* Exposed Function Declarations */
void parseBleDataPacket(le_advertising_info *info, BleDataPacket *bleDataPkt);
uint8_t getQaurtzEventFlag(le_advertising_info *info);
BlePacketType getBlePacketType(le_advertising_info *info);

#endif