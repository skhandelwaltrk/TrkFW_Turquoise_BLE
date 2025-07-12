#include <libconfig.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include "config.h"
#include "common.h"
#include "cloudComm.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SYS_CONFIG_FILE_PATH        "sysConfig.ini"

/* Connectable BLE Config Parameters */
bleConnectConfig bleConnectCfg = {0};
/* Cloud URL Config Parameters */
urlConfig urlCfg = {0};
/* Gateway Config Parameters */
gatewayConfig gwCfg = {0};

static char *dupOrNull(const char *str);

static char *dupOrNull(const char *str) {
    return str ? strdup(str) : NULL;
}

/* Function to format a MAC address as "11:22:33:44:55:66" */
char *formatMacAddress(const char *mac) {
    char *formattedMac = (char *)malloc(18);
    if (formattedMac == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    formattedMac[0] = '\0';

    for (int i = 0; i < 12; i += 2) {
        char byte[3] = {'\0'};

        byte[0] = (char)toupper((unsigned char)mac[i]);
        byte[1] = (char)toupper((unsigned char)mac[i + 1]);

        if (isxdigit((unsigned char)byte[0]) && isxdigit((unsigned char)byte[1])) {
            strcat(formattedMac, byte);
        } else {
            fprintf(stderr, "Invalid character in MAC address: %s\n", byte);
            free(formattedMac);
            exit(EXIT_FAILURE);
        }

        if (i < 10) {
            strcat(formattedMac, ":");
        }
    }

    return formattedMac;
}

int readSysConfigFile(void) {
    config_t cfg;
    config_init(&cfg);

    char absolute_path[PATH_MAX];
	const char *config_file_path = SYS_CONFIG_FILE_PATH;

	if (realpath(config_file_path, absolute_path) == NULL) {
		perror("realpath");
		exit(EXIT_FAILURE);
	}

	if (!config_read_file(&cfg, absolute_path)) {
		fprintf(stderr, "Error reading configuration file: %s\n", config_error_text(&cfg));
		config_destroy(&cfg);
		exit(EXIT_FAILURE);
	}

    //const char *instance;
    const char *gwLat = nullptr;
    const char *gwLon = nullptr;
    const char *urlExtension = nullptr;
    const char *urlAlive = nullptr;
    const char *instance = nullptr;
    const char *fwVersion = nullptr;
    const char *gwMacAddr = nullptr;
    int readTapeAgainDelaySecs = 10;

    config_setting_t *connectable_tape;

    if (config_lookup_string(&cfg, "instance", &instance) &&
        config_lookup_string(&cfg, "url_extension", &urlExtension) &&
        config_lookup_string(&cfg, "url_alive", &urlAlive) &&
        config_lookup_string(&cfg, "gw_latitude", &gwLat) &&
        config_lookup_string(&cfg, "gw_longitude", &gwLon) &&
        config_lookup_string(&cfg, "fw_version", &fwVersion) &&
        config_lookup_string(&cfg, "gw_mac_address", &gwMacAddr) &&
        config_lookup_int(&cfg, "ble_read_tape_again_delay", &readTapeAgainDelaySecs) &&
        (connectable_tape = config_lookup(&cfg, "ble_connectable_tapes")) != NULL)
	{
        urlCfg.instance = dupOrNull(instance);
        urlCfg.urlAlive = dupOrNull(urlAlive);
        urlCfg.urlExtension = dupOrNull(urlExtension);
        gwCfg.gwId = dupOrNull(gwMacAddr);
        gwCfg.gwLat = dupOrNull(gwLat);
        gwCfg.gwLon = dupOrNull(gwLon);
        gwCfg.fwVersion = dupOrNull(fwVersion);
        bleConnectCfg.readTapeAgainDelaySecs = readTapeAgainDelaySecs;

		TRK_PRINTF("=======================================================================================");
        TRK_PRINTF("%-25s = %s", "fw_version", gwCfg.fwVersion);
		TRK_PRINTF("%-25s = %s", "instance", urlCfg.instance);
		TRK_PRINTF("%-25s = %s", "gwLat", gwCfg.gwLat);
		TRK_PRINTF("%-25s = %s", "gwLon", gwCfg.gwLon);
		TRK_PRINTF("%-25s = %d", "read_tape_again_delay", bleConnectCfg.readTapeAgainDelaySecs);

        if (connectable_tape == NULL)
		{
			fprintf(stderr, "Error: 'connectable_tape' not found in the configuration file.\n");
			config_destroy(&cfg);
			exit(EXIT_FAILURE);
		}

		/* Get the number of elements in the connectable_tape array */
		bleConnectCfg.totalConnectableTapes = config_setting_length(connectable_tape);

		// Loop through the elements of the connectable_tape array and format and store MAC addresses
		for (int i = 0; i < bleConnectCfg.totalConnectableTapes; i++)
		{
            /* Get the i-th MAC address string from the config array */
            const char* connectableMac = config_setting_get_string_elem(connectable_tape, i);
            if (connectableMac == nullptr) continue;
            std::string macAddr = connectableMac;
            /* Format the MAC address to standard colon-separated format (e.g., E8:97:D2:28:F9:80) */
            formatMacAddrStr(macAddr);
            /* Skip if formatting failed or invalid MAC address */
            if (macAddr.empty()) continue;
            bleConnectCfg.tapeList[macAddr] = std::make_unique<tapeConfig>(macAddr, 0, false, false, 0, 0);
            TRK_PRINTF("BLE Connectable ID: %s", bleConnectCfg.tapeList[macAddr]->macAddr.c_str());
		}
		TRK_PRINTF("=======================================================================================");
    }
	else
	{
        fprintf(stderr, "Error: Missing or invalid configuration values\n");
        config_destroy(&cfg);
        exit(EXIT_FAILURE);
    }

    config_destroy(&cfg);
    return 0;
}

/**
 * @brief Retrieves the MAC address of a network interface.
 *
 * @param interface The name of the network interface (e.g., "wlan0").
 *                  Defaults to "wlan0" if not specified.
 * @return true if successful, false otherwise.
 */
bool getIfaceMacAddress(const char* interface) {
    if((interface==nullptr) || (std::strlen(interface)==0U)) {
        TRK_PRINTF("[getIfaceMacAddress] ERROR: Invalid input parameters");
        return false;
    }

    int fd=-1;
    struct ifreq ifr{};
    unsigned char* mac=nullptr;
    char macBuffer[18]={0};

    /* Create socket */
    fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd==-1) {
        TRK_PRINTF("[getIfaceMacAddress] ERROR: Internal setup failed");
        return false;
    }

    std::strncpy(ifr.ifr_name,interface,IFNAMSIZ-1U);
    ifr.ifr_name[IFNAMSIZ-1U]='\0';

    /* Fetch MAC address */
    if(ioctl(fd,SIOCGIFHWADDR,&ifr)==-1) {
        TRK_PRINTF("[getIfaceMacAddress] ERROR: MAC fetch failed");
        (void)close(fd);
        return false;
    }

    (void)close(fd);

    /* Format MAC address */
    mac=reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
    if(std::snprintf(macBuffer,sizeof(macBuffer),"%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5])<0) {
        TRK_PRINTF("[getIfaceMacAddress] ERROR: MAC format failed");
        return false;
    }

    /* Save the gateway MAC address to gateway configuration. */
    gwCfg.gwMacAddr = macBuffer;
    return true;
}