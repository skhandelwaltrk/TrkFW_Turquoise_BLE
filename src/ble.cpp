#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "ble.h"
#include "common.h"
#include "config.h"
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#define ATT_CID                             (4)

/* ----------------- BLE Synchronization Primitives ---------------------- */
/* Protects concurrent access to tapeList map across BLE scan and connect threads. */
std::mutex tapeListMutex;
/* Signals BLE connect thread when a connectable tape is marked (tapeFound = true). */
std::condition_variable tapeListCondVar; 

/* ----------------- Static Functions and Variables ---------------------- */
static bool verboseLogging  = true;
static inline std::string getBleMacIdFromAdvInfo(le_advertising_info *info);
static void updateTapeConnectionStatus(tapeConfig* tape, bool success);

/* -------- BLE Initialization Functions (called from the main thread) -------- */
void convertBdAddrToStr(bdaddr_t *addr, char *output) {
    int i, j = 0;
	char input[18];
	ba2str(addr, input);
    int len = strlen((const char *)input);

    for (i = 0; i < len; i++) {
        if (input[i] != ':') {
            output[j++] = input[i];
        }
    }

    // Null-terminate the output string
    output[j] = '\0';
}

int getGatewayBLEMacAddress(char *macAddress) {
	bdaddr_t bdaddr;
    int device_id = hci_get_route(NULL); // Get the device ID for the Bluetooth adapter

    if (device_id < 0) {
        TRK_PRINTF("HCI device get_route failed");
        return 1;
    }

    if (hci_devba(device_id, &bdaddr) < 0) {
        TRK_PRINTF("HCI device bdaddr retrieval failed");
        return 1;
    }
    int stat = 0;
    if (macAddress == nullptr) {
        stat = 1;
    }
    TRK_PRINTF("DBG: Converting BD Addr to Str. Stat:%d ", stat);
	convertBdAddrToStr(&bdaddr, macAddress);
    TRK_PRINTF("DBG: Converted BD Addr to Str");
    return 0;
}

/* ------------- BLE Scan Thread Functions (called from the main thread) ------------ */
static inline std::string getBleMacIdFromAdvInfo(le_advertising_info *info) {
    char addr[BLE_MAC_ADDR_LEN + 1] = {0};
    ba2str(&info->bdaddr, addr);
    return std::string(addr);
}

/**
 * @brief Checks if a scanned BLE advertisement corresponds to a connectable tape.
 *
 * This function extracts the MAC address from the BLE advertising information (`le_advertising_info`)
 * and looks it up in the map of connectable tapes. If the tape is found and its cooldown period has
 * expired (i.e., enough time has passed since the last transmission), the function marks it as
 * "found" so that a BLE connection can be established later.
 *
 * @param[in] info Pointer to the BLE advertising information structure received during scanning.
 *                 Must contain a valid Bluetooth address.
 *
 * @return true if the scanned device corresponds to a valid connectable tape and its cooldown period
 *         has expired; false otherwise.
 */
bool checkIfConnectableTape(le_advertising_info* info) {
    std::string scannedMacAddress = getBleMacIdFromAdvInfo(info);
    /* Check and format the scanned MAC address */
    formatMacAddrStr(scannedMacAddress);
    /* Check if the scanned device MAC address exists in the connectable tape list */
    auto it  = bleConnectCfg.tapeList.find(scannedMacAddress);
    if (it != bleConnectCfg.tapeList.end()) {
        time_t now = time(nullptr);

        // Check retry backoff
        if (now - it->second->lastSentTimeSecs >= it->second->backOffSecs &&
            now - it->second->lastSentTimeSecs >= bleConnectCfg.readTapeAgainDelaySecs)
        {
            it->second->tapeFound = true;
            /* Notify the BLE connector thread */
            tapeListCondVar.notify_one();
            return true;
        }
    }
    return false;
}

/* ----------------------- BLE Connect Thread Functions ------------------------- */

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
int connectToBleTape(const std::string dstMacAddr, uint8_t dst_type, int sec) {
	int sock;
	struct sockaddr_l2 srcaddr;
    struct sockaddr_l2 dstaddr;
	struct bt_security btsec;

	if (verboseLogging == true) {
		TRK_PRINTF("CONNECTABLE_BLE: Opening L2CAP LE connection on ATT "
            "channel:\n\t src: %s\n\tdest: %s\n", bleConnectCfg.gwBleMacId, dstMacAddr.c_str());
	}

	sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (sock < 0) {
		perror("Failed to create L2CAP socket");
        if (verboseLogging == true) {
            TRK_PRINTF("ERROR: Failed to create L2CAP socket");
        }
		return -1;
	}

	/* Set up source address */
	memset(&srcaddr, 0, sizeof(srcaddr));
	srcaddr.l2_family = AF_BLUETOOTH;
	srcaddr.l2_cid = htobs(ATT_CID);
	srcaddr.l2_bdaddr_type = 0;
    str2ba(bleConnectCfg.gwBleMacId, &srcaddr.l2_bdaddr);

	if (bind(sock, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) < 0) {
		perror("Failed to bind L2CAP socket");
        if (verboseLogging == true) {
            TRK_PRINTF("ERROR: Failed to bind L2CAP socket");
        }
		close(sock);
		return -1;
	}

	/* Set the security level */
	memset(&btsec, 0, sizeof(btsec));
	btsec.level = sec;
	if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &btsec,
							sizeof(btsec)) != 0) {
		fprintf(stderr, "Failed to set L2CAP security level\n");
        if (verboseLogging == true) {
            TRK_PRINTF("ERROR: Failed to set L2CAP security level");
        }
		close(sock);
		return -1;
	}

	/* Set up destination address */
	memset(&dstaddr, 0, sizeof(dstaddr));
	dstaddr.l2_family = AF_BLUETOOTH;
	dstaddr.l2_cid = htobs(ATT_CID);
	dstaddr.l2_bdaddr_type = dst_type;
	str2ba(dstMacAddr.c_str(), &dstaddr.l2_bdaddr);

	TRK_PRINTF("Connecting to device ...");
	fflush(stdout);

	if (connect(sock, (struct sockaddr *) &dstaddr, sizeof(dstaddr)) < 0) {
        TRK_PRINTF("Failed to connect to the BLE device %s", dstMacAddr.c_str());
		close(sock);
		return -1;
	}

	TRK_PRINTF("Connected to the BLE device %s", dstMacAddr.c_str());
	return sock;
}

/**
 * @brief Updates the connection status, retry count, backoff delay, and timestamp for a tape.
 *
 * @param tape    Pointer to the tapeConfig struct to update.
 * @param success True if the BLE connection was successful; false otherwise.
 */
static void updateTapeConnectionStatus(tapeConfig* tape, bool success) {
    tape->lastSentTimeSecs = time(nullptr);

    if (success) {
        tape->tapeConnected = true;
        tape->retryCount = 0;
        tape->backOffSecs = 0;
    } else {
        tape->tapeConnected = false;
        tape->retryCount++;
        /* linear backoff capped at 60s */
        tape->backOffSecs = std::min(60, tape->retryCount * 5);  
    }
}

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
void bleConnectThreadFunc() {
    while (keepRunning) {
        std::unique_lock<std::mutex> bleConnectLock(tapeListMutex);

        /* Wait until at least one tape is marked for connection */
        tapeListCondVar.wait(bleConnectLock, [] {
            for (const auto& entry : bleConnectCfg.tapeList) {
                if (entry.second->tapeFound) return true;
            }
            return false;
        });

        for (auto it = bleConnectCfg.tapeList.begin(); it != bleConnectCfg.tapeList.end(); ++it) {
            auto& tapeCfg = it->second;
            const std::string& tapeMacAddr = it->first;

            /* Do not try to establish connection for the connectable tapes not found while scanning */
            if (tapeCfg->tapeFound == false)
                continue;

            /* Set the tape found flag as false to disable multiple connection attempts going forward */
            tapeCfg->tapeFound = false;
            bleConnectLock.unlock();

            TRK_PRINTF("Initiating connection to the BLE device: %s", tapeMacAddr.c_str());
            bool isTapeConnected = connectToBleTape(tapeMacAddr);

            bleConnectLock.lock();

            updateTapeConnectionStatus(tapeCfg.get(), isTapeConnected);

            if (isTapeConnected) {
                TRK_PRINTF("Connected successfully to: %s", tapeMacAddr.c_str());
            } else {
                TRK_PRINTF("Connection failed: %s, retry=%d, backoff=%d",
                            tapeMacAddr.c_str(), tapeCfg->retryCount, tapeCfg->backOffSecs);
            }
        }
    }
}
