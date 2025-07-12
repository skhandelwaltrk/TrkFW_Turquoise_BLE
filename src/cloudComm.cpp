#include <cstdint>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <iostream>
#include "common.h"
#include "cloudComm.h"
#include "config.h"

using namespace std;

static void createCloudDataUrl(char* urlBuff, size_t urlBuffLen, const char* instance, const char* pageUrl, const char* dataBuff, size_t dataBuffLen);
static int createBaseUrlLink(char* urlBuff, size_t urlBuffLen, const char* instance, const char* pageUrl);
static int addBleDataToBaseUrl(char* urlBuff, size_t urlBuffLen, const char* dataBuff, size_t dataBuffLen);

/* Function to append pageUrl to instance and store in buffer */
static int createBaseUrlLink(char* urlBuff, size_t urlBuffLen, const char* instance, const char* pageUrl) {
    if (instance == nullptr || pageUrl == nullptr || urlBuff == nullptr) {
        TRK_PRINTF("ERROR: URL link could not be created!");
        return -1;
    }

    size_t len1 = strlen(instance);
    size_t len2 = strlen(pageUrl);

    if (len1 + len2 + 1 > urlBuffLen) {
        TRK_PRINTF("ERROR: URL Buffer too small! Required: %zu, Provided: %zu", len1 + len2 + 1, urlBuffLen);
        return -1;
    }

    strcpy(urlBuff, instance);
    strcat(urlBuff, pageUrl);

    return 0;
}

/* Function to append dataBuff to urlBuff (mutable!) */
static int addBleDataToBaseUrl(char* urlBuff, size_t urlBuffLen, const char* dataBuff, size_t dataBuffLen) {
    if (urlBuff == nullptr || dataBuff == nullptr) {
        TRK_PRINTF("ERROR: Invalid params - URL Data link could not be created!");
        return -1;
    }

    size_t len1 = strlen(urlBuff);
    if (len1 + dataBuffLen + 1 > urlBuffLen) {
        TRK_PRINTF("ERROR: URL Buffer too small! Required: %zu, Provided: %zu", len1 + dataBuffLen + 1, urlBuffLen);
        return -1;
    }

    strncat(urlBuff, dataBuff, dataBuffLen);
    return 0;
}

/* Master function to create full URL with data */
static void createCloudDataUrl(char* urlBuff, size_t urlBuffLen, const char* instance, const char* pageUrl, const char* dataBuff, size_t dataBuffLen) {
    if (dataBuff == nullptr || dataBuffLen == 0) {
        TRK_PRINTF("ERROR: Invalid params, could not create cloud packets");
        return;
    }

    if (createBaseUrlLink(urlBuff, urlBuffLen, instance, pageUrl) != 0) {
        TRK_PRINTF("ERROR: Failed to create the base URL.");
        return;
    }

    if (addBleDataToBaseUrl(urlBuff, urlBuffLen, dataBuff, dataBuffLen) != 0) {
        TRK_PRINTF("ERROR: Failed to add BLE data to the URL.");
        return;
    }
}

static size_t curlReqWriteCb(void *respData, size_t size, size_t nmemb, void *userp) {
    size_t totalSize = size * nmemb;
    strncat((char *)userp, (char *)respData, totalSize);
    return totalSize;
}

const char* getGwId(void) {
    return gwCfg.gwId;
}

/* Send the data URL to the cloud */
int sendDataUrlToCloud(const char *packetDataBuff, size_t packetDataLen) {
    if (packetDataBuff == nullptr || packetDataLen == 0) {
        TRK_PRINTF("ERROR: Invalid input params - Send URL to cloud failed");
        return -1;
    }
    static CURL* curl = nullptr;
    static bool initialized = false;
    static uint32_t curlProcessCount = 0;
    int err_ret;    
    /* Retrieve and print the transfer time and DNS lookup time */
    double total_time = 0.0;
    //char CallString[1024];
    char urlBuff[MAX_URL_LEN];
    memset(urlBuff, 0, MAX_URL_LEN);

    /* Create the Cloud Data URL that contains the BLE data. */
    createCloudDataUrl(urlBuff, MAX_URL_LEN, urlCfg.instance, urlCfg.urlExtension, packetDataBuff, packetDataLen);

    /* Response data container */
    string response_data;

    if (!initialized) {
        err_ret = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (err_ret != 0) {
            TRK_PRINTF("Curl_Proc: failed curl_global_init");
            return 1;
        }

        curl = curl_easy_init();
        if (!curl) {
            curl_global_cleanup();
            TRK_PRINTF("Curl_Proc: failed curl_easy_init");
            return 1;
        }

        /* Set the DNS cache timeout */
        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, CURL_REQUEST_DNS_CACHE_TIMEOUT_SECS);

        initialized = true;
    }

    TRK_PRINTF("Curl_Proc %d: curl rqst going=%s", ++curlProcessCount, urlBuff);
    err_ret = curl_easy_setopt(curl, CURLOPT_URL, urlBuff);
    if (err_ret != 0) {
        TRK_PRINTF("Curl_Proc: failed set CURLOPT_URL");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        initialized = false;
        curl = nullptr;
        return 1;
    }

    err_ret = curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_REQUEST_TIMEOUT_SECS);
    if (err_ret != 0) {
        TRK_PRINTF("Curl_Proc: failed set CURLOPT_TIMEOUT");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        initialized = false;
        curl = nullptr;
        return 1;
    }

    err_ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlReqWriteCb);
    if (err_ret != 0) {
        TRK_PRINTF("Curl_Proc: failed set CURLOPT_WRITEFUNCTION");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        initialized = false;
        curl = nullptr;
        return 1;
    }

    err_ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    if (err_ret != 0) {
        TRK_PRINTF("Curl_Proc: failed set CURLOPT_WRITEDATA");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        initialized = false;
        curl = nullptr;
        return 1;
    }

    /* Perform the request */
    CURLcode res = curl_easy_perform(curl);

    /* Check if the curl request was sucessful or not */
    if (res != CURLE_OK) {
        TRK_PRINTF("Curl_Proc: curl_easy_perform() failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        initialized = false;
        curl = nullptr;
        return 1;
    }
    else {
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code == 200) {
            TRK_PRINTF("Curl_Proc %d: Received HTTP 200 OK", curlProcessCount);
        }
        else {
            TRK_PRINTF("Curl_Proc %d: Received HTTP response code: %ld", curlProcessCount, response_code);
        }
        //TRK_PRINTF("Curl_Proc: Curl rqst %d successful, Response: %s", curlProcessCount, response_data.c_str());
    }

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
    /* Print the response data along with timings in one line */
    TRK_PRINTF("Curl_Proc %d: Response data: %s, Total transfer time: %.3f seconds", 
    curlProcessCount, response_data.c_str(), total_time);

    return 0;
}
