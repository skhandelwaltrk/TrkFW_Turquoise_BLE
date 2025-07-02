#ifndef _COMMON_H_
#define _COMMON_H_

#include <cstdio>
#include <cstdint>
#include <ctime>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "tapeFormat.h"

using namespace std;


/* Program Exit Reasons */
#define EXIT_ERR_INVALID_ARGS                     (-2)
#define EXIT_ERR_NULL_PTR                         (-4)

/* BLE Advertise and Scan Parameters */
#define BLE_SCAN_INIT_RETRY_LIMIT                 (3u)
#define BLE_SCAN_TIME_INTERVALS_SEC               (20u)
#define BLE_RSSI_THRESHOLD                        (-80)

#define HCI_DEV_ID                                (0u)
#define MAX_BEACONS_SCANNED                       (400u)
#define MAC_ADDR_LEN                              (6u)

#define SLEEP_MSECS(ms)                           (usleep(ms * 1000))
#define SLEEP_SECS(s)                             (sleep(s))

#define TRK_PRINTF1(fmt, ...)          \
  do {                                \
    printf(fmt "\n", ##__VA_ARGS__);  \
    fflush(stdout);                   \
  } while (0)
//

#define TRK_PRINTF(...)  \
  do {                   \
    printf(__VA_ARGS__); \
    fflush(stdout);      \
  } while (0)
//

/* Quartz BLE Data Packet Size: 24 bytes; Byte 25: RSSI
   Total packet size: 25 bytes */
typedef struct QuartzHeartbeatData {
    char mac_addr[20];
    uint8_t fid1;             // 0
    uint8_t fid2;             // 1
    uint8_t evt_flag;         // 2
    int8_t curr_temp;         // 3
    uint8_t temp_violation;   // 4
    uint32_t t1_ts;           // 5 - 8
    uint8_t a0_val;           // 9
    uint8_t a0_count;         // 10
    uint32_t a0_ts;           // 11 - 14
    uint16_t pid1;            // 15 - 16
    uint32_t ts;              // 17 - 20
    uint16_t tapeId;          // 21 -22
    float bat_voltage;       // 23
    int8_t rssi;              // 24
} QuartzHeartbeatData;

typedef enum e_QuartzEventFlag {
  NormalMode                    = 60,
  InMotionMode                  = 61,
  RunningToSleepMode            = 2,
  EnteringAeroplaneMode         = 4,
  InAeroplaneMode               = 5,
  OutOffAeroplaneMode           = 6,
  EnteringWhiteTapeMode         = 7,
  InWhiteTapeMode               = 17,
  OutOffWhiteTapeMode           = 8,
  EnteringHibernationMode       = 9,
  InHibernationMode             = 62,
  OutOffHibernationMode         = 10,
  CutCircuitResetMode           = 11,
  WatchDogResetMode             = 12,
  ButtonPressResetMode          = 13,
  CPULockResetMode              = 14,
  SoftwareInstructionResetMode  = 15,
  OtherResetsMode               = 16,
  EnteringShutdownMode          = 19,
  InShutdownMode                = 20,
  ExitingShutDownMode           = 22,
  HeartbeatMode                 = 21,
  ConfigMode                    = 66,
  TemperatureHeartbeatMode      = 50,
  TemperatureViolationMode      = 63
} e_QuartzEventFlag;

//extern queue<BleDataPacket> bleDataQueue;
//extern mutex bleQueueMutex;
//extern condition_variable bleQueueCondVar;
extern bool keepRunning;

/* Inline functions */
inline time_t getEpochTimeSecs(void) {
  return time(nullptr);
}

/* ----------------------------- Function Declarations ----------------------------- */
//void printBlePacketData(BleDataPacket *bleData);
//void createBleDataUrlExtension(char *urlDataBuff, uint16_t urlDataBuffLen, BleDataPacket *blePkt);
e_QuartzEventFlag UpdateEventFlagForQuartz(uint8_t evt_flag);
void sysInit(void);
#endif /* _COMMON_H_ */