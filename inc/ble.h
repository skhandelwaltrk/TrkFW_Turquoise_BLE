#ifndef _BLE_H_
#define _BLE_H_

#include <string>
#include <map>
#include <functional>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

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

int getGatewayBLEMacAddress(char *macAddress);
void convertBdAddrToStr(bdaddr_t *addr, char *output);

/**
 * @brief Checks if a scanned BLE advertisement corresponds to a connectable tape.
 *
 * This function extracts the MAC address from the BLE advertising information (`le_advertising_info`)
 * and looks it up in the map of connectable tapes. If the tape is found and its cooldown period has
 * expired (i.e., enough time has passed since the last transmission), the function marks it as
 * "found" so that a BLE connection can be established later.
 *
 * @param[in] info Pointer to the BLE advertising information structure received during scanning.
 *                 Must contain a valid Blxuetooth address.
 *
 * @return true if the scanned device corresponds to a valid connectable tape and its cooldown period
 *         has expired; false otherwise.
 */
bool checkIfConnectableTape(le_advertising_info* info);

/**
 * @brief Initiates a Bluetooth LE connection to a remote BLE device by MAC address.
 *
 * This function establishes a BLE L2CAP connection on the ATT (Attribute) channel
 * using the specified Bluetooth MAC address of a remote device. It opens a BLE L2CAP
 * socket, binds it to the local adapter, applies the desired security level, and
 * connects to the remote device over the ATT CID (0x0004).
 *
 * @param[in] dstMacAddr MAC address of the destination BLE device in string format.
 *                       Must be a valid 17-character colon-separated address.
 * @param[in] dst_type Address type of the destination device (default: BDADDR_LE_RANDOM).
 *                     Use 0 for public or 1 for random.
 * @param[in] sec Desired Bluetooth security level (default: BT_SECURITY_LOW).
 *                Options include BT_SECURITY_LOW, BT_SECURITY_MEDIUM, or BT_SECURITY_HIGH.
 *
 * @return int Socket file descriptor if the connection is successful, or -1 on failure.
 */
int connectToBleTape(const std::string dstMacAddr, uint8_t dst_type = BDADDR_LE_RANDOM, int sec = BT_SECURITY_LOW);

/**
 * @brief Thread function that attempts BLE connections to known connectable tapes.
 *
 * This function runs in a loop, waiting for connectable tapes to be marked (via `tapeFound`)
 * by the BLE scan thread. When a tape is ready, it attempts to establish a BLE connection.
 * Connection results update the tape's retry count, backoff delay, and last sent timestamp.
 * Uses a condition variable to efficiently sleep until new connection candidates are available.
 *
 * @note This function is intended to be run in its own thread.
 */
void bleConnectThreadFunc();

#endif /* _BLE_H */