#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <cstdint>
#include <map>
#include <string>
#include <memory>
#include <cctype>

extern int totalConnectableTapes;

#define GW_LOC_BUFF_SIZE                (25)
#define BLE_CONNECT_TYPE_PUBLIC         

typedef struct tapeConfig {
    std::string macAddr;                 /* MAC address string */
    uint32_t lastSentTimeSecs = 0;       /* Time in seconds since last send */
    bool tapeFound = false;              /* Flag to indicate if tape was seen in latest scan */
    bool tapeConnected = false;          /* Connection status */
    int retryCount = 0;                  /* Number of connection retries */
    int backOffSecs = 0;                 /* Backoff time in seconds */

    /* Default constructor */
    tapeConfig() = default;

    /* Parameterized constructor */
    tapeConfig(const std::string& addr,
               uint32_t time,
               bool found,
               bool connected,
               int rCount,
               int bSecs)
        : macAddr(addr),
          lastSentTimeSecs(time),
          tapeFound(found),
          tapeConnected(connected),
          retryCount(rCount),
          backOffSecs(bSecs) {}
} tapeConfig;

typedef struct urlConfig {
    const char *urlAlive;
    const char *urlExtension;
    const char *instance;
} urlConfig;

typedef struct gatewayConfig {
    const char *gwId;
    const char *gwLat;
    const char *gwLon;
    const char *fwVersion;
} gatewayConfig;

typedef struct bleConnectConfig {
    int totalConnectableTapes;
    int readTapeAgainDelaySecs;
    char *gwBleMacId;
    std::map<std::string, std::unique_ptr<tapeConfig>> tapeList;
} bleConnectConfig;

extern bleConnectConfig bleConnectCfg;
extern urlConfig urlCfg;
extern gatewayConfig gwCfg;

/* Function to format a MAC address as "11:22:33:44:55:66" */
char *formatMacAddress(const char *mac);
int readSysConfigFile(void);

/**
 * @brief Checks if the input string is a valid colon-formatted MAC address ("XX:XX:XX:XX:XX:XX").
 *
 * @param mac Input MAC string.
 * @return true if the format is correct, false otherwise.
 */
inline bool isFormattedMacAddr(const std::string& mac) {
    if (mac.size() != 17) return false;
    for (size_t i = 0; i < 17; ++i) {
        if ((i % 3) == 2) {
            if (mac[i] != ':') return false;
        } else {
            if (!std::isxdigit(static_cast<unsigned char>(mac[i]))) return false;
        }
    }
    return true;
}

/**
 * @brief Checks if the input string is a valid raw 12-digit hexadecimal MAC address (no colons).
 *
 * @param mac Input MAC string.
 * @return true if the input is valid raw hex, false otherwise.
 */
static inline bool isRawMacAddress(const std::string& mac) {
    if (mac.size() != 12) return false;
    for (char c : mac) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

/**
 * @brief Converts a raw or formatted MAC string into colon-separated format.
 *
 *        - If the input is already colon-formatted and valid, it's left unchanged.
 *        - If it's a valid 12-digit hex string, it's formatted.
 *        - If it's invalid, the string is cleared.
 *
 * @param macStr Reference to the MAC address string to format in place.
 */
inline void formatMacAddrStr(std::string& macStr) {
    if (isFormattedMacAddr(macStr)) {
        return;
    }

    if (isRawMacAddress(macStr)) {
        std::string formatted;
        for (size_t i = 0; i < 12; i += 2) {
            formatted += macStr.substr(i, 2);
            if (i < 10) formatted += ':';
        }
        macStr = formatted;
        return;
    }

    /* Invalid MAC Address format */
    macStr.clear();
}

#endif /* _CONFIG_H_ */