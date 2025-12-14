/*! \file swadgePass.h
 *
 * \section swadgePass_design Design Philosophy
 *
 * SwadgePass is a feature where Swadges can transmit and receive small packets of data while idle. This feature was
 * inspired by Nintendo's [StreetPass](https://en.wikipedia.org/wiki/SpotPass_and_StreetPass). SwadgePass data may
 * include things like avatar data or high scores from all Swadge Modes.
 *
 * SwadgePass uses ESP-NOW, not WiFi or Bluetooth. See hdw-esp-now.h.
 *
 * \subsection swadgePass_tx_rx Transmission and Reception
 *
 * Packets are transmitted with sendSwadgePass(). To preserve battery life, this should only be called when the Swadge
 * is idling in it's LED animation mode (`dance.c`). In this state, the Swadge will transmit every 7.5s, +/- 1.875s.
 * Randomness ensures two Swadges don't get locked out-of-sync while transmitting and receiving. After transmitting, the
 * Swadge stays awake and listening for 700ms. While not transmitting or listening, the Swadge light sleeps for 100ms
 * increments, updating the LEDs ten times per second.
 *
 * The receiver is initialized with initSwadgePassReceiver() and packets are received with receiveSwadgePass(). Any
 * incoming packet may be passed to this function, including from modes which are using ESP-NOW for other purposes,
 * as long as the receiver was initialized. deinitSwadgePassReceiver() frees memory allocated for the receiver.
 *
 * During normal operation, if not explicitly used the WiFi radio is turned off and SwadgePass data is neither
 * transmitted nor received.
 *
 * \section swadgePass_usage Usage
 *
 * Transmission and reception of SwadgePass data is automatic in `dance.c` and does not need to be managed by each
 * SwadgeMode (see \ref swadgePass_tx_rx). Other modes may transmit and receive SwadgePass data, but it is discouraged
 * for battery reasons.
 *
 * \subsection sp_building_packet Building a Packet for Transmission
 *
 * A ::swadgePassPacket_t is built with the function fillSwadgePassPacket(), which calls each mode's
 * swadgeMode_t.fnAddToSwadgePassPacket function. Each mode may add data to ::swadgePassPacket_t, and modes must not
 modify data which is not their own. The total packet size must be less than 250 bytes, which is the largest possible
 ESP-NOW packet.
 *
 * \warning
 * swadgeMode_t.fnAddToSwadgePassPacket is called when the Swadge Mode is not initialized, so it **must not** rely on
 * memory allocated or data loaded in swadgeMode_t.fnEnterMode
 *
 * \subsection sp_using_packets Using Received SwadgePass Data
 *
 * Swadge Modes may check received SwadgePass data by giving an empty ::list_t to getSwadgePasses() to fill.
 * getSwadgePasses() may fill the list with all received SwadgePass data, or only SwadgePass data which has not been
 * used by the given mode yet. After being filled, the list contain pointers to ::swadgePassData_t and may be
 * iterated through. swadgePassData_t.key is the string representation of the source MAC address. SwadgePass data will
 * not change during Swadge Modes (aside from `dance.c`), so the data should be loaded once and saved for the mode's
 * lifetime. getSwadgePasses() should not be called repeatedly.
 *
 * If the Swadge Mode wants to use each SwadgePass data from a source only once, it can be checked with
 * isPacketUsedByMode(). This may be useful for an RPG game where each received SwadgePass gets to make one action. Once
 * the data is used, it can be marked as such with setPacketUsedByMode(). This will save the used state to non-volatile
 * storage, which persists reboots.
 *
 * When the Swadge Mode is finished with the SwadgePass data, it must be freed with freeSwadgePasses().
 *
 * \section swadgePass_example Example
 *
 * \subsection sp_building_packet_example Building a Packet for Transmission Example
 *
 * swadgeMode_t.fnAddToSwadgePassPacket would be set to this example function.
 *
 * \code{.c}
 * void myModeAddToSwadgePassPacket(struct swadgePassPacket* packet)
 * {
 *     // The field name should indicate which mode the data is for
 *     // This is an example, your high score probably shouldn't be hardcoded
 *     packet->myHighScore = 9001;
 * }
 * \endcode
 *
 * \subsection sp_using_packets_example Using Received SwadgePass Data Example
 *
 * \code{.c}
 * // Get all SwadgePasses, including used ones
 * list_t spList = {0};
 * getSwadgePasses(&spList, &myMode, true);
 *
 * // Iterate through the list
 * node_t* spNode = spList.first;
 * while (spNode)
 * {
 *     // Make a convenience pointer to the data in this node
 *     swadgePassData_t* spd = (swadgePassData_t*)spNode->val;
 *
 *     // If the data hasn't been used yet
 *     if (!isPacketUsedByMode(spd, &myMode))
 *     {
 *         // Print some packet data
 *         ESP_LOGI("SP", "Receive from %s. Preamble is %d", spd->key, spd->data.packet.preamble);
 *
 *         // Mark the packet as used
 *         setPacketUsedByMode(spd, &myMode, true);
 *     }
 *
 *     // Iterate to the next data
 *     spNode = spNode->next;
 * }
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "swadgesona.h"

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A SwadgePass packet which is transmitted over ESP NOW
 */
typedef struct __attribute__((packed)) swadgePassPacket
{
    uint16_t preamble; ///< Two bytes that specifically begin a SwadgePass packet
    uint8_t version;   ///< A version byte to differentiate packets per-year
    struct
    {
        uint16_t highScore;
    } cosCrunch;
    struct
    {
        int8_t reactHs;
        int8_t memHs;
    } swadgeIt;
    struct
    {
        uint16_t highScore;
    } roboRunner;
    struct
    {
        swadgesonaCore_t core;
    } swadgesona;

    struct {
        int cardSelect;     // Active card
        int fact0;       
        int fact1;
        int fact2;
        uint16_t numPasses; // Number of other unique passes encountered
    } atrium;
} swadgePassPacket_t;

/**
 * @brief SwadgePass data that is saved to NVS.
 */
typedef struct
{
    uint32_t usedModeMask;     ///< A bitmask indicating if a mode has used this data
    swadgePassPacket_t packet; ///< The received SwadgePass packet
} swadgePassNvs_t;

/**
 * @brief SwadgePass data that is received from another Swadge
 */
typedef struct
{
    char key[NVS_KEY_NAME_MAX_SIZE]; ///< A string representation of the other Swadge's MAC address
    swadgePassNvs_t data;            ///< The SwadgePass data stored in this Swadge's NVS
} swadgePassData_t;

//==============================================================================
// Forward declaration
//==============================================================================

struct swadgeMode;

//==============================================================================
// Function Declarations
//==============================================================================

void initSwadgePassReceiver(void);
void deinitSwadgePassReceiver(void);

void fillSwadgePassPacket(swadgePassPacket_t* packet);
void sendSwadgePass(swadgePassPacket_t* packet);
void receiveSwadgePass(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);

void getSwadgePasses(list_t* swadgePasses, const struct swadgeMode* mode, bool getUsed);
void freeSwadgePasses(list_t* swadgePasses);

bool isPacketUsedByMode(swadgePassData_t* data, const struct swadgeMode* mode);
void setPacketUsedByMode(swadgePassData_t* data, const struct swadgeMode* mode, bool isUsed);