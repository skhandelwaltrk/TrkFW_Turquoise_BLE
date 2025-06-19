#ifndef _CLOUDCOMM_H_
#define _CLOUDCOMM_H_

#include <cstdio>
#include <cstdint>

#define CURL_REQUEST_TIMEOUT_SECS                 (30)
#define CURL_REQUEST_DNS_CACHE_TIMEOUT_SECS       (600L)
#define MAX_URL_LEN                               (512)

#define FACTORY_INSTANCE                          "https://trksbxmanuf.azure-api.net/internal"
#define TURQUOISE_GW_ID                           "124678807272"

/* Send the data URL to the cloud */
int sendDataUrlToCloud(const char *packetDataBuff, size_t packetDataLen);
const char* getGwId(void);

#endif /* _CLOUDCOMM_H_ */