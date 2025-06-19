#ifndef _BLE_H_
#define _BLE_H_

#include <string>
#include <map>
#include <functional>

using namespace std;

typedef enum {
    BLE_SCAN_STATE_IDLE,
    BLE_SCAN_STATE_SCANNING,
} BLE_SCAN_STATES;

typedef enum {
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_WHITE,
    DEVICE_TYPE_LIME,
    DEVICE_TYPE_ULD,
    DEVICE_TYPE_SPSF_GW,
    DEVICE_TYPE_MAX,
} device_type_t;

typedef struct {
    // instantaneous rssi
    int rssi;
    // total rssi
    int totalRssi;
    // avg rssi
    int avgRssi;
    // seen count
    int seenCount;
    // type of device seen
    device_type_t deviceType;
} BleScanRecord;

typedef bool (*ScanResultCallback)(std::map<std::string, BleScanRecord> &);

void startContinuousScan(uint32_t scanDurationSec, uint32_t sleepDurationSec);
void startOneShotScan(uint32_t scanDurationSec);
bool stopBleScan();

#endif /* _BLE_H */