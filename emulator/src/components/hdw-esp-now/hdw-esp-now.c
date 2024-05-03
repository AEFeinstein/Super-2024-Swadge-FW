//==============================================================================
// Includes
//==============================================================================

// clang-format off
#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) \
                     || defined(WIN32)       || defined(WIN64) \
                     || defined(_WIN32)      || defined(_WIN64) \
                     || defined(__WIN32__)   || defined(__CYGWIN__) \
                     || defined(__MINGW32__) || defined(__MINGW64__) \
                     || defined(__TOS_WIN__) || defined(_MSC_VER)
    #define USING_WINDOWS 1
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
    #define USING_LINUX 1
#elif __APPLE__
    #define USING_MAC 1
#else
    #error "OS Not Detected"
#endif
// clang-format on

#if defined(USING_WINDOWS)
    #include <WinSock2.h>
#elif defined(USING_LINUX) || defined(USING_MAC)
    #include <sys/socket.h> // for socket(), connect(), sendto(), and recvfrom()
    #include <arpa/inet.h>  // for sockaddr_in and inet_addr()
    #include <fcntl.h>
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "hdw-esp-now.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define ESP_NOW_PORT  32888
#define MAXRECVSTRING 1024 // Longest string to receive

//==============================================================================
// Variables
//==============================================================================

hostEspNowRecvCb_t hostEspNowRecvCb = NULL;
hostEspNowSendCb_t hostEspNowSendCb = NULL;

int socketFd;

//==============================================================================
// Functions
//==============================================================================

/**
 * Initialize ESP-NOW and attach callback functions.
 * This uses wifi by default, but espNowUseSerial() may be called later to
 * communicate over the given UART instead
 *
 * @param recvCb A callback to call when data is sent
 * @param sendCb A callback to call when data is received
 * @param rx The receive pin when using serial communication instead of wifi. Use GPIO_NUM_NC for no GPIO
 * @param tx The transmit pin when using serial communication instead of wifi. Use GPIO_NUM_NC for no GPIO
 * @param uart The UART to use for serial communication. Use UART_NUM_MAX for no UART
 * @param wifiMode The WiFi mode. If ESP_NOW_IMMEDIATE, then recvCb is called directly from the interrupt. If ESP_NOW,
 * then recvCb is called from checkEspNowRxQueue()
 */
esp_err_t initEspNow(hostEspNowRecvCb_t recvCb, hostEspNowSendCb_t sendCb, gpio_num_t rx, gpio_num_t tx,
                     uart_port_t uart, wifiMode_t wifiMode)
{
    // Save callbacks
    hostEspNowRecvCb = recvCb;
    hostEspNowSendCb = sendCb;

#if defined(USING_WINDOWS)
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ESP_LOGE("WIFI", "WSAStartup failed");
        return ESP_ERR_WIFI_IF;
    }
#endif

    // Create a best-effort datagram socket using UDP
    if ((socketFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        ESP_LOGE("WIFI", "socket() failed");
        return ESP_ERR_WIFI_IF;
    }

    // Set socket to allow broadcast
    int broadcastPermission = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, (void*)&broadcastPermission, sizeof(broadcastPermission)) < 0)
    {
        ESP_LOGE("WIFI", "setsockopt() failed");
        return ESP_ERR_WIFI_IF;
    }

    // Allow multiple sockets to bind to the same port
    int enable = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (void*)&enable, sizeof(int)) < 0)
    {
        ESP_LOGE("WIFI", "setsockopt() failed");
        return ESP_ERR_WIFI_IF;
    }

#if defined(USING_WINDOWS)
    //-------------------------
    // Set the socket I/O mode: In this case FIONBIO
    // enables or disables the blocking mode for the
    // socket based on the numerical value of iMode.
    // If iMode = 0, blocking is enabled;
    // If iMode != 0, non-blocking mode is enabled.
    u_long iMode = 1;
    if (ioctlsocket(socketFd, FIONBIO, &iMode) != 0)
    {
        ESP_LOGE("WIFI", "ioctlsocket() failed");
        return ESP_ERR_WIFI_IF;
    }
#else
    int optval_enable = 1;
    setsockopt(socketFd, SOL_SOCKET, O_NONBLOCK, (char*)&optval_enable, sizeof(optval_enable));
#endif

    // Set nonblocking timeout
    struct timeval read_timeout;
    read_timeout.tv_sec  = 0;
    read_timeout.tv_usec = 10;
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&read_timeout, sizeof(read_timeout));

    // Construct bind structure
    struct sockaddr_in broadcastAddr;                    // Broadcast Address
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));    // Zero out structure
    broadcastAddr.sin_family      = AF_INET;             // Internet address family
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);   // Any incoming interface
    broadcastAddr.sin_port        = htons(ESP_NOW_PORT); // Broadcast port

    // Bind to the broadcast port
    if (bind(socketFd, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0)
    {
        ESP_LOGE("WIFI", "bind() failed");
        return ESP_ERR_WIFI_IF;
    }
    return ESP_OK;
}

/**
 * Start wifi and use it for communication
 */
esp_err_t espNowUseWireless(void)
{
    // Do nothing
    return ESP_OK;
}

/**
 * Start the UART and use it for communication
 *
 * @param crossoverPins true to crossover the rx and tx pins, false to use them
 *                      as normal.
 */
void espNowUseSerial(bool crossoverPins)
{
    // Do nothing
}

/**
 * Check the ESP NOW receive queue. If there are any received packets, send
 * them to hostEspNowRecvCb()
 */
void checkEspNowRxQueue(void)
{
    char recvString[MAXRECVSTRING + 1]; // Buffer for received string
    int recvStringLen;                  // Length of received string

    // While we've received a packet
    while ((recvStringLen = recvfrom(socketFd, recvString, MAXRECVSTRING, 0, NULL, 0)) > 0)
    {
        // If the packet matches the ESP_NOW format
        uint8_t recvMac[6] = {0};
        if (6
            == sscanf(recvString, "ESP_NOW-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX-", &recvMac[0], &recvMac[1],
                      &recvMac[2], &recvMac[3], &recvMac[4], &recvMac[5]))
        {
            // Make sure the MAC differs from our own
            uint8_t ourMac[6] = {0};
            esp_wifi_get_mac(WIFI_IF_STA, ourMac);
            if (0 != memcmp(recvMac, ourMac, sizeof(ourMac)))
            {
                // Set up the receive info
                esp_now_recv_info_t espNowInfo = {0};
                espNowInfo.src_addr            = recvMac;
                espNowInfo.des_addr            = ourMac;

                wifi_pkt_rx_ctrl_t packetRxCtrl = {0};
                packetRxCtrl.rssi               = 0x7F;
                espNowInfo.rx_ctrl              = &packetRxCtrl;

                // If it does, send it to the application through the callback
                hostEspNowRecvCb(&espNowInfo, (uint8_t*)&recvString[21], recvStringLen - 21, packetRxCtrl.rssi);
            }
        }
    }
}

/**
 * This is a wrapper for esp_now_send(). It also sets the wifi power with
 * wifi_set_user_fixed_rate()
 *
 * @param data The data to broadcast using ESP NOW
 * @param len  The length of the data to broadcast
 */
void espNowSend(const char* data, uint8_t dataLen)
{
    struct sockaddr_in broadcastAddr; // Broadcast address

    // Construct local address structure
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));    // Zero out structure
    broadcastAddr.sin_family      = AF_INET;             // Internet address family
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_NONE);  // Broadcast IP address  // inet_addr("255.255.255.255");
    broadcastAddr.sin_port        = htons(ESP_NOW_PORT); // Broadcast port

    // Tack on ESP-NOW header and randomized MAC address
    char espNowPacket[dataLen + 24];
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(espNowPacket, "ESP_NOW-%02X%02X%02X%02X%02X%02X-", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    int hdrLen = strlen(espNowPacket);
    memcpy(&espNowPacket[hdrLen], data, dataLen);

    // For the callback
    uint8_t bcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    errno = 0;
    // Send the packet
    int sentLen
        = sendto(socketFd, espNowPacket, hdrLen + dataLen, 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    if (sentLen != (hdrLen + dataLen))
    {
        ESP_LOGE("WIFI", "sendto() sent a different number of bytes than expected: %d, not %d", sentLen,
                 hdrLen + dataLen);
        if (errno != 0)
        {
            ESP_LOGE("WIFI", "errno was: %d", errno);
        }

        hostEspNowSendCb(bcastMac, ESP_NOW_SEND_FAIL);
    }
    else
    {
        hostEspNowSendCb(bcastMac, ESP_NOW_SEND_SUCCESS);
    }
}

/**
 * This function is called to de-initialize ESP-NOW
 */
void deinitEspNow(void)
{
    close(socketFd);
#if defined(USING_WINDOWS)
    WSACleanup();
#endif
}
