#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include "unistd.h"
#include <fcntl.h>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <thread>
#include <cstdlib>
#include <csignal>
#include "ble.h"
#include "common.h"
#include "cloudComm.h"
#include "tapeFormat.h"
#include "config.h"

using namespace std;

/* Global variables */
queue<BleDataPacket> bleDataQueue;
mutex bleQueueMutex;
condition_variable bleQueueCondVar;
bool keepRunning = true;

struct BleScanOptions {
    volatile bool continuous = false;
    volatile uint32_t scanDurationSec = 0;
    volatile uint32_t sleepDurationSec = 0;
    volatile ScanResultCallback callback;

    void clear()
    {
        continuous = false;
        scanDurationSec = 0;
        sleepDurationSec = 0;
        callback = nullptr;
    }
};

/* Local Variables */
atomic<BLE_SCAN_STATES> bleScanState(BLE_SCAN_STATE_IDLE);
atomic<bool> scanStopRequested = false;
BleScanOptions scanOptions;

int hciDevUp() {
    int ctl, ret = 0;
    /* Open HCI socket  */
    ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (ctl < 0) {
        cout << "Opening HCI socket: " << string(strerror(errno)) << endl;
        return ctl;
    }

    ret = ioctl(ctl, HCIDEVUP, HCI_DEV_ID);
    if (ret < 0) {
        if (errno == EALREADY) {
            cout << "HCI device is already up" << endl;
            // if the interface is already up, we consider that as a success
            ret = 0;
        } else {
            cout << "HCIDEVUP call failed: " << string(strerror(errno)) << endl;
        }
        close(ctl);
        return ret;
    }

    close(ctl);
    return 0;
}

int hciDevDown() {
    int ctl, ret = 0;
    /* Open HCI socket  */
    ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (ctl < 0) {
        cout << "HCI socket: " << string(strerror(errno)) << endl;
        return ctl;
    }

    ret = ioctl(ctl, HCIDEVDOWN, HCI_DEV_ID);
    if (ret < 0) {
        cout << "HCIDEVDOWN call failed: " << string(strerror(errno)) << endl;
        close(ctl);
        return ret;
    }

    close(ctl);
    return 0;
}

void hciDevReset() {
    hciDevDown();
    SLEEP_MSECS(500);
    hciDevUp();
}

static bool setScanFilters(int fd) {
    uint8_t le_type = 0x00;
    // 40ms scan interval
    uint16_t le_scan_interval = htobs(0x0040);
    // 30ms scan window
    uint16_t le_scan_window = htobs(0x0030);
    // Public device address
    uint8_t le_own_bdaddr_type = 0x00;
    // No whitelist filtering — scan all devices
    uint8_t le_filter = 0x00;

    int ret = hci_le_set_scan_parameters(fd, le_type, le_scan_interval, le_scan_window, le_own_bdaddr_type, le_filter,
                                         BLE_SCAN_TIME_INTERVALS_SEC);
    if (ret < 0) {
        cout << "ERROR: Set scan parameters: " << string(strerror(errno)) << endl;
        //incrementBleMetrics(BLE_METRIC_NUM_SET_SCAN_PARAMS_FAIL);
        return false;
    }

    return true;
}

static bool configureHciFilter(int fd) {
    struct hci_filter filter;
    hci_filter_clear(&filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &filter);

    if (setsockopt(fd, SOL_HCI, HCI_FILTER, &filter, sizeof(filter)) < 0) {
        cout << "ERROR: Failed to set HCI socket filter: " << string(strerror(errno)) << endl;
        // incrementBleMetrics(BLE_METRIC_NUM_SET_FILTER_FAIL);
        return false;
    }

    return true;
}

static bool enableDisableBleScan(int fd, bool enable) {
    // enable scan
    uint8_t le_scan_enable = enable;
    uint8_t le_scan_filter_dup = 0x01;
    uint8_t scan_time = 0x00;

    int ret = hci_le_set_scan_enable(fd, le_scan_enable, le_scan_filter_dup, scan_time);
    if (ret < 0) {
        cout << "ERROR: " << string(enable ? "Enable" : "Disable") << " ble scan failed: " << string(strerror(errno)) << endl;
        // incrementBleMetrics(BLE_METRIC_NUM_ENABLE_FAILS);
        return false;
    }

    if (le_scan_enable == false) {
        bleScanState.store(BLE_SCAN_STATE_IDLE);
    }

    return true;
}

static bool initBleScan(int &fd) {
    for (uint8_t retry_count = 0; retry_count < BLE_SCAN_INIT_RETRY_LIMIT; retry_count++) {
        fd = hci_open_dev(HCI_DEV_ID);
        if (fd < 0) {
            cout << "ERROR: Opening HCI device: " << string(strerror(errno)) << endl;
            // incrementBleMetrics(BLE_METRIC_NUM_OPEN_DEV_FAIL);
            SLEEP_MSECS(100);
            hciDevReset();
            continue;
        }

        if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
            cout << "ERROR: Setting O_NONBLOCK failed: " << string(strerror(errno)) << endl;
            // incrementBleMetrics(BLE_METRIC_NUM_FCNTL_FAIL);
            close(fd);
            hciDevReset();
            SLEEP_MSECS(100);
            continue;
        }

        if (!configureHciFilter(fd) || !setScanFilters(fd) || !enableDisableBleScan(fd, true)) {
            cout << "ERROR: BLE scan setup failed, retry: " << string(strerror(errno)) << endl;
            close(fd);
            hciDevReset();
            SLEEP_MSECS(100);
            continue;
        }

        return true;
    }

    cout << "ERROR: BLE scan init failed after all retries" << endl;
    return false;
}

bool startBleScan(BleScanOptions &options) {
    if (bleScanState.load() != BLE_SCAN_STATE_IDLE) {
        cout << "ERROR: Scan in progress" << endl;
        return false;
    }

    scanOptions = options;
    scanStopRequested.store(false);
    // trkSemPost(__func__, &bleScanSem);
    bleScanState.store(BLE_SCAN_STATE_SCANNING);
    return true;
}

bool stopBleScan() {
    if (bleScanState.load() != BLE_SCAN_STATE_SCANNING) {
        cout << "ERROR: Scan already stopped" << endl;
        // scan already stopped
        return true;
    }

    scanStopRequested.store(true);
    // trkSemPost(__func__, &bleScanSem);
    bleScanState.store(BLE_SCAN_STATE_IDLE);
    return true;
}

void startOneShotScan(uint32_t scanDurationSec) {
    BleScanOptions scanOptions;
    scanOptions.continuous = false;
    scanOptions.scanDurationSec = scanDurationSec;
    //scanOptions.callback = cb;
    startBleScan(scanOptions);
}

void startContinuousScan(uint32_t scanDurationSec, uint32_t sleepDurationSec) {
    BleScanOptions scanOptions;
    scanOptions.continuous = true;
    scanOptions.scanDurationSec = scanDurationSec;
    scanOptions.sleepDurationSec = sleepDurationSec;
    //scanOptions.callback = cb;
    startBleScan(scanOptions);
}

bool isBleRssiInRange(int8_t rssi) {
    return rssi >= BLE_RSSI_THRESHOLD;
}

inline bool getInfoDataAt(const le_advertising_info* info, uint8_t index, uint8_t& outValue) {
    if (!info || index >= info->length) {
        return false;
    }
    outValue = info->data[index];
    return true;
}

/**
 * @brief Converts a 2-byte epoch timestamp into a formatted date-time string.
 *
 * This function takes high and low bytes representing a 16-bit epoch timestamp
 * (seconds since 1970-01-01 00:00:00 UTC) and returns a formatted string in
 * the format "MM-DD-YYYY HH:MM:SS".
 *
 * @param buffer The data buffer pointer of the 16-bit epoch value.
 * @return A string representing the local time in "MM-DD-YYYY HH:MM:SS" format.
 *
 * @note The 16-bit epoch value has a max range of 0–65535 seconds (~18.2 hours).
 *       For larger time ranges, use a wider timestamp format.
 */
inline string getTimeFromEpoch(const uint8_t* buffer) {
    if (!buffer) return "Invalid";

    // Convert 4-byte big-endian buffer to uint32_t epoch time
    uint32_t epoch = (static_cast<uint32_t>(buffer[0]) << 24) |
                     (static_cast<uint32_t>(buffer[1]) << 16) |
                     (static_cast<uint32_t>(buffer[2]) << 8)  |
                     (static_cast<uint32_t>(buffer[3]));

    time_t fullEpoch = static_cast<time_t>(epoch);

    struct tm timeInfo {};
    localtime_r(&fullEpoch, &timeInfo);  // Thread-safe

    ostringstream oss;
    oss << setfill('0')
        << setw(2) << timeInfo.tm_mon + 1 << "-"
        << setw(2) << timeInfo.tm_mday << "-"
        << setw(4) << timeInfo.tm_year + 1900 << " "
        << setw(2) << timeInfo.tm_hour << ":"
        << setw(2) << timeInfo.tm_min << ":"
        << setw(2) << timeInfo.tm_sec;

    return oss.str();
}

e_QuartzEventFlag UpdateEventFlagForQuartz(uint8_t evt_flag) {
    switch (evt_flag) {
        case EVT_QUARTZ_TMP117_NORMAL_MODE:
            return NormalMode;
        case EVT_WHITE_TAPE_MOTION_MODE:
            return InMotionMode;
        case EVT_WHITE_TAPE_HIBERNATION_MODE:
            return InHibernationMode;
        case EVT_WHITE_TAPE_TEMP_VIOLATION_MODE:
            return TemperatureViolationMode;
        default:
            return (e_QuartzEventFlag)evt_flag;
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

device_type_t getBleDataSource(le_advertising_info *info) {
    if (info == nullptr) {
        TRK_PRINTF("\nERROR: Invalid input parameters - BLE source could not be determined!");
        return DEVICE_TYPE_UNKNOWN;
    }

    device_type_t deviceType  = DEVICE_TYPE_MAX;
    uint8_t trkCompanyIdHigh  = 0;
    uint8_t trkCompanyIdLow   = 0;
    uint8_t trackHigh         = 0;
    uint8_t trackLow          = 0;
    uint8_t trkIdHigh         = 0;
    uint8_t trkIdLow          = 0;
    uint8_t rfidCompanyIdHigh = 0;
    uint8_t rfidCompanyIdLow  = 0;
    uint8_t rfidTrkIdHigh     = 0;
    uint8_t rfidTrkIdLow      = 0;
    uint8_t rfidProdIdHigh    = 0;
    uint8_t rfidProdIdLow     = 0;
    bool isTrkNordicDevice    = false;
    bool isTrkRfidDevice      = false;

    if (!getInfoDataAt(info, COMPANY_IDENTIFIER_HIGH_INDEX, trkCompanyIdHigh)) return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, COMPANY_IDENTIFIER_LOW_INDEX,  trkCompanyIdLow))  return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, TRACK_IDENTIFIER_HIGH_INDEX,   trkIdHigh))        return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, TRACK_IDENTIFIER_LOW_INDEX,    trkIdLow))         return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, RFID_GW_COMPANY_IDENTIFIER_HIGH_INDEX, rfidCompanyIdHigh)) return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, RFID_GW_COMPANY_IDENTIFIER_LOW_INDEX,  rfidCompanyIdLow))  return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, RFID_GW_TRACK_IDENTIFIER_HIGH_INDEX,   rfidTrkIdHigh))     return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, RFID_GW_TRACK_IDENTIFIER_LOW_INDEX,    rfidTrkIdLow))      return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, RFID_GW_PRODUCT_ID_HIGH_INDEX,         rfidProdIdHigh))    return DEVICE_TYPE_UNKNOWN;
    if (!getInfoDataAt(info, RFID_GW_PRODUCT_ID_LOW_INDEX,          rfidProdIdLow))     return DEVICE_TYPE_UNKNOWN;

    isTrkNordicDevice = (trkCompanyIdHigh == NORDIC_IDENTIFIER_HIGH_BYTE) &&
                             (trkCompanyIdLow == NORDIC_IDENTIFIER_LOW_BYTE);

    /* For rfid gw, ble advertisement(refer trkble for format) does not contain the flags which exists in white tape advertisement. 
    So adv is offset by 3 bytes, So we parse for both the cases */
    isTrkRfidDevice =
            (rfidCompanyIdHigh == NORDIC_IDENTIFIER_HIGH_BYTE) && (rfidCompanyIdLow == NORDIC_IDENTIFIER_LOW_BYTE) &&
            (rfidProdIdHigh == RFID_GW_PRODUCT_ID_HIGH_BYTE) && (rfidProdIdLow == RFID_GW_PRODUCT_ID_LOW_BYTE);

    if (!isTrkNordicDevice && !isTrkRfidDevice) {
        // Not a recognized device
        return DEVICE_TYPE_UNKNOWN;
    }

    // Use the appropriate track identifier
    trackHigh = isTrkNordicDevice ? trkIdHigh : rfidTrkIdHigh;
    trackLow = isTrkNordicDevice ? trkIdLow : rfidTrkIdLow;

    if ((trackHigh == LIME_MILESTONE_HIGH_BYTE) && (trackLow == LIME_MILESTONE_LOW_BYTE)) {
        deviceType = DEVICE_TYPE_LIME;
    }

    if ((trackHigh == WHITETAPE_HIGH_BYTE) && (trackLow == WHITETAPE_LOW_BYTE)) {
        deviceType = isTrkRfidDevice ? DEVICE_TYPE_SPSF_GW : DEVICE_TYPE_WHITE;
    }

    if ((trackHigh == ULD_HIGH_BYTE) && (trackLow == ULD_LOW_BYTE)) {
        deviceType = DEVICE_TYPE_ULD;
    }

    if (deviceType != DEVICE_TYPE_WHITE || info->length < 30) {
        deviceType = DEVICE_TYPE_UNKNOWN;
    }

    return deviceType;
}

bool isValidWhiteTapeBleSource(le_advertising_info *info, device_type_t& deviceType) {
    if (info == nullptr) {
        TRK_PRINTF1("ERROR: Null Ptr - BLE source check failed!");
        return false;
    }

    deviceType = getBleDataSource(info);
    char macAddr[20];
    memset(macAddr, 0, sizeof(macAddr));
    ba2str(&info->bdaddr, macAddr);
    removeChar(macAddr,':');

    if ((strcmp(macAddr, "E897D628F980") == 0) && deviceType == DEVICE_TYPE_WHITE && info->length > 30) {
        TRK_PRINTF1("TapeData:MAC_ADDR:%s,", macAddr);
        int i = 0;
        for (i = 0; i < info->length; i++) {
            printf("[%d]:0x%02X",i,info->data[i]);
        }
        printf("\n");
    }

    return ((deviceType == DEVICE_TYPE_WHITE) && (isBleRssiInRange(info->data[info->length]) == true));
}

/*!
    @brief: Checks and updates the BLE Stats for the Tape.
*/
void checkAndUpdateBleStats(le_advertising_info *info, map<string, BleScanRecord> &scanResult, device_type_t deviceType) {

    if (info == nullptr) {
        TRK_PRINTF1("ERROR: Null Ptr - Cannot check and update BLE stats");
        return;
    }

    /* Get the MAC address */
    char mac_addr[20];
    memset(mac_addr, 0, sizeof(mac_addr));
    ba2str(&info->bdaddr, mac_addr);
    removeChar(mac_addr,':');
    int8_t tapeRssi = getTapeRssi(info);

    auto it = scanResult.find(mac_addr);
    if (it != scanResult.end()) {
        it->second.rssi = tapeRssi;
        it->second.seenCount += 1;
        it->second.totalRssi += tapeRssi;
        it->second.avgRssi = (it->second.totalRssi) / it->second.seenCount;
    } else {
        BleScanRecord record;
        record.rssi = tapeRssi;
        record.totalRssi = tapeRssi;
        record.avgRssi = tapeRssi;
        record.seenCount = 1;
        record.deviceType = deviceType;
        scanResult[mac_addr] = record;
    }
}

bool isBleContScanEnabled(void) {
    return scanOptions.continuous;
}

void printBlePacketData(BleDataPacket *bleData) {
    if (bleData == nullptr) {
        return;
    }
    /* Update this print later -
    TRK_PRINTF("\nBLE_PKT:MAC:%s,FID:0x%02X%02X,e0:%u,t0:%d.%d,ts:%d,a0_v:%d,a0_c:%u,a0_ts:%d,pid1:%d,ts:%d,tapeID:%d,bat_volt:%f,rssi:%d", 
        bleData->mac_addr, bleData->fid1, bleData->fid2, bleData->evt_flag, bleData->curr_temp, 
        bleData->temp_violation, bleData->t1_ts, bleData->a0_val, bleData->a0_count, bleData->a0_ts,
        bleData->pid1, bleData->ts, bleData->tapeId, bleData->bat_voltage, bleData->rssi);
    */
}

int createBleDataUrlExtension(char *urlDataBuff, size_t urlDataBuffLen, BleDataPacket *blePkt) {
    if (urlDataBuff == nullptr || urlDataBuffLen < 128 || blePkt == nullptr) {
        TRK_PRINTF1("ERROR: Invalid input parameters, BLE data URL extension creation failed!");
        return -1;
    }
    static char gwLat[] = "40.7064738";
    static char gwLon[] = "-74.0103607";
    static int seqNumber[QuartzSensor_Max] = {0};

    memset(urlDataBuff, 0, urlDataBuffLen);

    switch(blePkt->blePktType) {
        case QuartzSensor_TMP117: {
            BlePacket_QuartzTMP117 *bleTmp117 = &blePkt->blePktStrct.blePkt_TMP117;
            snprintf(urlDataBuff, urlDataBuffLen, "?gid=%s&ts=%d&C=%d&id=%s&rssi=%d"
            "&e0=%d&t0=%.2f&t1=%.2f&t1ts=%d&t2=%.2f&t2ts=%d&pid=%d&seqId=%d&tapeId=0x%04X"
            "&bat=%.3f&clat=%s&clon=%s&type=%s&st=5261", getGwId(), bleTmp117->t0_ts,
            ++seqNumber[QuartzSensor_TMP117], bleTmp117->mac_addr, bleTmp117->rssi, bleTmp117->evt_flag,
            bleTmp117->t0, bleTmp117->t1, bleTmp117->t1_ts, bleTmp117->t2, bleTmp117->t2_ts,
            bleTmp117->pid, bleTmp117->seqId, bleTmp117->tapeId, bleTmp117->bat, gwLat, gwLon, getGwId());
            return URL_CREATE_SUCCESS;
        }
        case QuartzSensor_OPT3110: {
            BlePacket_QuartzOPT3110 *bleOpt3110 = &blePkt->blePktStrct.blePkt_OPT3110;
            snprintf(urlDataBuff, urlDataBuffLen, "?gid=%s&ts=%d&C=%d&id=%s&rssi=%d"
            "&e0=%d&t0=%.2f&l0=%d&l0ts=%d&l1=%d&l1ts=%d&pid=%d&lbat=%.2f&seqId=%d"
            "&tapeId=0x%04X&bat=%.3f&clat=%s&clon=%s&type=%s&st=5261", getGwId(), bleOpt3110->t0_ts,
            ++seqNumber[QuartzSensor_OPT3110], bleOpt3110->mac_addr, bleOpt3110->rssi, bleOpt3110->evt_flag,
            bleOpt3110->t0, bleOpt3110->l0, bleOpt3110->l0_ts, bleOpt3110->l1, bleOpt3110->l1_ts, bleOpt3110->pid,
            (((float)bleOpt3110->lime_bat)/100.0f), bleOpt3110->seqId, bleOpt3110->tapeId, bleOpt3110->bat,
            gwLat, gwLon, getGwId());
            return URL_CREATE_SUCCESS;
        }
        case QuartzSensor_IAT: {
            BlePacket_IAT *bleIAT = &blePkt->blePktStrct.blePkt_IAT;
            snprintf(urlDataBuff, urlDataBuffLen, "?gid=%s&ts=%d&C=%d&id=%s&rssi=%d"
            "&e0=%d&t0=%.2f&t1=%.2f&t1ts=%d&l0=%d&l0ts=%d&a0v=%d&a0c=%d&tapeId=0x%04X"
            "&bat=%.3f&clat=%s&clon=%s&type=%s&st=5261", getGwId(), bleIAT->ts,
            ++seqNumber[QuartzSensor_IAT], bleIAT->mac_addr, bleIAT->rssi, bleIAT->evt_flag,
            bleIAT->t0, bleIAT->t1, bleIAT->t1_ts, bleIAT->l0, bleIAT->l0_ts, bleIAT->a0_val,
            bleIAT->a0_count, bleIAT->tapeId, bleIAT->bat, gwLat, gwLon, getGwId());
            return URL_CREATE_SUCCESS;
        }
        case QuartzSensor_DPD: {
            BlePacket_DPD *bleDPD = &blePkt->blePktStrct.blePkt_DPD;
            snprintf(urlDataBuff, urlDataBuffLen, "?gid=%s&ts=%d&C=%d&id=%s&rssi=%d"
            "&e0=%d&t0=%.2f&t1=%.2f&t1ts=%d&l0=%d&l0ts=%d&l1=%d&tapeId=0x%04X"
            "&bat=%.3f&clat=%s&clon=%s&type=%s&st=5261", getGwId(), bleDPD->ts,
            ++seqNumber[QuartzSensor_DPD], bleDPD->mac_addr, bleDPD->rssi, bleDPD->evt_flag,
            bleDPD->t0, bleDPD->t1, bleDPD->t1_ts, bleDPD->l0, bleDPD->l0_ts, bleDPD->l1,
            bleDPD->tapeId, bleDPD->bat, gwLat, gwLon, getGwId());
            return URL_CREATE_SUCCESS;
        }
        default: {
            //TRK_PRINTF1("ERROR: Unknown Ble Packet Type detected, blePktType: %d", blePkt->blePktType);
            return -2;
        }
    }
}

void sendBleDataPacket(BleDataPacket& bleDataPkt) {
    lock_guard<mutex> lock(bleQueueMutex);
    bleDataQueue.push(bleDataPkt);
    bleQueueCondVar.notify_one();
}

/* BLE Thread Function */
void bleScanThreadFunc(uint32_t bleScanTime, uint32_t bleSleepTime) {
    int fd = -1;
    uint32_t elapsedSec = 0;
    int ret = 0;
    const uint8_t retry_delay_sec = 5;
    const uint8_t init_retry_log_throttle_sec = 30;
    map<string, BleScanRecord> scanResults;

    TRK_PRINTF("\nStarted BLE Thread ...");

    ret = hciDevUp();
    if (ret < 0) {
        /* try once more by doing hci down and then hci up */
        hciDevReset();
    }

    /* Get the Gateway BLE MAC Address */
    getGatewayBLEMacAddress(bleConnectCfg.gwBleMacId);
    TRK_PRINTF1("BLE GW MAC ID: %s", bleConnectCfg.gwBleMacId);

    while (keepRunning) {
        ret = initBleScan(fd);
        if ((ret == 0) || (fd < 0)) {
            cout << "ERROR: Failed to init ble scan after retries" << init_retry_log_throttle_sec << endl;
            SLEEP_SECS(retry_delay_sec);
            continue;
        }

        /* Set the parameters for scanning the */
        startContinuousScan(bleScanTime, bleSleepTime);

        elapsedSec = 0;
        cout << "BLE scan started for: " << to_string(scanOptions.scanDurationSec) << "s" << endl;

        while (elapsedSec < scanOptions.scanDurationSec) {
            if (scanStopRequested.load()) {
                break;
            }

            SLEEP_SECS(1);
            elapsedSec++;
        }

        TRK_PRINTF1("Ble scan completed");

        uint8_t buf[HCI_MAX_EVENT_SIZE];
        while (keepRunning) {
            /* Struct to send over BLE packet data to the cloud communication thread */
            int len = 0;
            BleDataPacket blePacketData;
            device_type_t deviceType = DEVICE_TYPE_UNKNOWN;
            le_advertising_info *info = nullptr;
            evt_le_meta_event *meta_event = nullptr;

            memset(buf, 0, sizeof(buf));
            len = read(fd, buf, sizeof(buf));
            if (len <= 0) {
                break;
            } 
            else if (len < HCI_EVENT_HDR_SIZE) {
                // incrementBleMetrics(BLE_METRIC_NUM_SCAN_ERRORS);
                TRK_PRINTF1("ERROR: Failed to read from hci");
                break;
            }

            meta_event = (evt_le_meta_event *)(buf + HCI_EVENT_HDR_SIZE + 1);
            if (meta_event->subevent != EVT_LE_ADVERTISING_REPORT) {
                continue;
            }

            info = (le_advertising_info *)(meta_event->data + 1);
            if (info == nullptr) {
                continue;
            }

            /* Check if the data received is for the Quartz White Tape */
            if (isValidWhiteTapeBleSource(info, deviceType) == false) {
                continue;
            }

            /* Check if the scanned tape is in the connectable BLE list */
            //checkIfConnectableTape(info);

            /* Check and update the BLE stats for the white tape. */
            checkAndUpdateBleStats(info, scanResults, deviceType);
            
            /* Run the dups logic (Define the below function )*/
            // if (isNotDuplicateBleData(info, scanResults) != PASSED) {
            //    continue;
            // }

            /* Parse the BLE data based on the tape ID and create packet for sending data to the cloud */
            parseBleDataPacket(info, &blePacketData);

            /* Send the data to the cloud, create a queue and add data to it. 
               Cloud communication thread can communicate with the cloud and 
               send the data. */
            char *tapeMacAddr = blePacketData.blePktStrct.blePkt_TMP117.mac_addr;
            if (strcmp(tapeMacAddr, "E897D628F980") == 0) {
                //TRK_PRINTF1("Sending TMP117 BLE packet for MAC:%s ...", tapeMacAddr);
                sendBleDataPacket(blePacketData);
#if 0
                BleDataPacket blePacketData_OPT3110, blePacketData_IAT, blePacketData_DPD;
                blePacketData_OPT3110.blePktType = QuartzSensor_OPT3110;
                blePacketData_IAT.blePktType = QuartzSensor_IAT;
                blePacketData_DPD.blePktType = QuartzSensor_DPD;
                uint8_t tBuff[sizeof(le_advertising_info) + 256] = {0};
                le_advertising_info *tinfo = (le_advertising_info *)tBuff;
                /* Create OPT3110 info data */
                bacpy(&tinfo->bdaddr, &info->bdaddr);
                //TRK_PRINTF1("OPT3110:tinfo->bdaddr:%s, info->bdaddr:%s",tinfo->bdaddr, info->bdaddr);
                memset(tinfo->data, 0, 32);
                tinfo->data[7] = 0x52;
                tinfo->data[8] = 0x58;
                tinfo->data[9] = 0; // e0 = Normal Mode (55)
                tinfo->data[10] = 22;
                tinfo->data[11] = 58;
                tinfo->data[12] = 0x12;
                tinfo->data[13] = 0x34;
                tinfo->data[14] = 0x56;
                tinfo->data[15] = 0x78;
                tinfo->data[16] = 0x13;
                tinfo->data[17] = 0x57;
                tinfo->data[18] = 0x57;
                tinfo->data[19] = 0x9B;
                tinfo->data[20] = 0x24;
                tinfo->data[21] = 0x68;
                tinfo->data[22] = 0x12;
                tinfo->data[23] = 0x34;
                tinfo->data[24] = 0x56;
                tinfo->data[25] = 151;
                tinfo->data[26] = 0x00;
                tinfo->data[27] = 0x02;
                tinfo->data[28] = 0xFF;
                tinfo->data[29] = 0xFA;
                tinfo->data[30] = 32;
                tinfo->data[31] = 59;
                tinfo->length = 31;

                parseBleDataPacket(tinfo, &blePacketData_OPT3110);
                TRK_PRINTF1("Sending OPT3110 BLE packet for MAC:%s ...", tapeMacAddr);
                sendBleDataPacket(blePacketData_OPT3110);

                /* Create IAT info data */
                memset(tinfo->data, 0, 32);
                tinfo->data[7] = 0x52;
                tinfo->data[8] = 0x58;
                tinfo->data[9] = 0; // e0 = Normal Mode (55)
                tinfo->data[10] = 22;
                tinfo->data[11] = 61;
                tinfo->data[12] = 0x12;
                tinfo->data[13] = 0x34;
                tinfo->data[14] = 0x56;
                tinfo->data[15] = 0x78;
                tinfo->data[16] = 0x13;
                tinfo->data[17] = 0x57;
                tinfo->data[18] = 0x12;
                tinfo->data[19] = 0x34;
                tinfo->data[20] = 0x56;
                tinfo->data[21] = 0x78;
                tinfo->data[22] = 0x12;
                tinfo->data[23] = 0x34;
                tinfo->data[24] = 0x12;
                tinfo->data[25] = 0x34;
                tinfo->data[26] = 0x56;
                tinfo->data[27] = 0x78;
                tinfo->data[28] = 0xFF;
                tinfo->data[29] = 0xB1;
                tinfo->data[30] = 31;
                tinfo->data[31] = 60;
                tinfo->length = 31;

                parseBleDataPacket(tinfo, &blePacketData_IAT);
                TRK_PRINTF1("Sending IAT BLE packet for MAC:%s ...", tapeMacAddr);
                sendBleDataPacket(blePacketData_IAT);

                /* Create DPD info data */
                memset(tinfo->data, 0, 32);
                tinfo->data[7] = 0x52;
                tinfo->data[8] = 0x58;
                tinfo->data[9] = 0; // e0 = Normal Mode (55)
                tinfo->data[10] = 22;
                tinfo->data[11] = 61;
                tinfo->data[12] = 0x12;
                tinfo->data[13] = 0x34;
                tinfo->data[14] = 0x56;
                tinfo->data[15] = 0x78;
                tinfo->data[16] = 0x13;
                tinfo->data[17] = 0x57;
                tinfo->data[18] = 0x12;
                tinfo->data[19] = 0x34;
                tinfo->data[20] = 0x56;
                tinfo->data[21] = 0x78;
                tinfo->data[22] = 0x12;
                tinfo->data[23] = 0x34;
                tinfo->data[24] = 0x12;
                tinfo->data[25] = 0x34;
                tinfo->data[26] = 0x56;
                tinfo->data[27] = 0x78;
                tinfo->data[28] = 0xFF;
                tinfo->data[29] = 0xB0;
                tinfo->data[30] = 30;
                tinfo->data[31] = 61;
                tinfo->length = 31;

                parseBleDataPacket(tinfo, &blePacketData_DPD);
                TRK_PRINTF1("Sending DPD BLE packet for MAC:%s ...", tapeMacAddr);
                sendBleDataPacket(blePacketData_DPD);
#endif
            }

            /*
                if ((strcmp(bleData->mac_addr, "E897D628F980") == 0) && deviceType == DEVICE_TYPE_WHITE && info->length > 30) {
                    TRK_PRINTF("BLE_STATS:Device:%d,Bytes:%d,MAC:%s,RSSI:%d,TotalRSSI:%d,AvgRSSI:%d,SeenCnt:%d\n",
                    scanResult[bleData->mac_addr].deviceType, info->length, bleData->mac_addr, scanResult[bleData->mac_addr].rssi,
                    scanResult[bleData->mac_addr].totalRssi, scanResult[bleData->mac_addr].avgRssi, 
                    scanResult[bleData->mac_addr].seenCount);
                    int i = 0;
                    TRK_PRINTF("BLE_DATA:MAC:%s", bleData->mac_addr);
                    for (i = 0; i <= info->length; i++) {
                        TRK_PRINTF(",[%d]:0x%02X", i, info->data[i]);
                    }
                    TRK_PRINTF(",RSSI:%d\n", (int8_t)info->data[info->length]);
                }
            */
        }

        enableDisableBleScan(fd, false);
        close(fd);

        if (isBleContScanEnabled() == false) {
            TRK_PRINTF1("One shot BLE scan mode enabled, Stopping BLE activity ...");
            scanOptions.clear();
            return;
        }

        /* BLE sleep period */
        TRK_PRINTF1("BLE Sleep Started ...");
        SLEEP_MSECS(scanOptions.sleepDurationSec);
        TRK_PRINTF1("BLE Sleep Stopped!");
    }
}

/* Cloud Communication Thread Function */
void cloudCommicationThreadFunc() {
    TRK_PRINTF1("Started Cloud Communication Thread ...");

    while (keepRunning) {
        unique_lock<mutex> lock(bleQueueMutex);
        bleQueueCondVar.wait(lock, [] {
            return !bleDataQueue.empty() || !keepRunning;
        });

        while (!bleDataQueue.empty()) {
            /* Fetch the BLE data packet from the queue */
            BleDataPacket blePkt = bleDataQueue.front();
            bleDataQueue.pop();
            lock.unlock();
            char dataBuff[256] = {0};
            /* Create the URL externsion that contains the BLE data */
            int urlCreateStatus = createBleDataUrlExtension(dataBuff, sizeof(dataBuff), &blePkt);
            if (urlCreateStatus == URL_CREATE_SUCCESS) {
                /* Send the Data URL to the cloud */
                sendDataUrlToCloud(dataBuff, strlen(dataBuff) + 1);
            }
            else {
                //TRK_PRINTF1("ERROR: Failed to create URL for BLE packet, Status: %d", urlCreateStatus);
            }
            lock.lock();
        }
    }
}

/* Signal handler for the keyboard Interrupt */
void keyboardIrqHandler(int signum) {
    TRK_PRINTF("\nReceived signal:%d, exiting...", signum);
    keepRunning = false;
    bleQueueCondVar.notify_all();
}

void sysInit(void) {
    readSysConfigFile();
}

int main(int argc, char *argv[]) {
    /* Check and parse the input arguments - To be removed later */
    if (argc < 3) {
        TRK_PRINTF1("ERROR: Invalid Input Parameters");
        return -1;
    }

    uint32_t bleScanTime = (uint32_t)atoi(argv[1]);
    uint32_t bleSleepTime = (uint32_t)atoi(argv[2]);

    /* Setup signal to handle keyboard interrupt */
    signal(SIGINT, keyboardIrqHandler);

    /* Initialize the system parameters and fetch the system configuration */
    sysInit();

    /* Create thread to communicate to the cloud */
    thread cloudCommThread(cloudCommicationThreadFunc);
    thread bleScanThread(bleScanThreadFunc, bleScanTime, bleSleepTime);
    //thread bleConnectThread(bleConnectThreadFunc);

    /* The main thread sleeps for 1 second and checks the running status */
    while(keepRunning) {
        /* Sleep for 1 second */
        sleep(1);
    }

    cloudCommThread.join();
    bleScanThread.join();
    //bleConnectThread.join();

    TRK_PRINTF1("Program exited cleanly");
    return 0;
}