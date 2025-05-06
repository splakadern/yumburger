// Header File: type.h
#ifndef TYPE_H
#define TYPE_H

// Include necessary ESP-IDF Wi-Fi types
#include <esp_wifi.h>
#include <esp_wifi_types.h> // Often needed for structures like wifi_pkt_rx_ctrl_t and wifi_promiscuous_pkt_t

// --- Custom Structure Definitions ---

// Structure for a basic 802.11 MAC header (simplified for deauth context)
typedef struct {
  uint16_t frame_ctrl;    // Frame Control (2 bytes)
  uint16_t duration;      // Duration/ID (2 bytes)
  uint8_t addr1[6];       // Address 1 (Destination MAC / Receiver MAC)
  uint8_t addr2[6];       // Address 2 (Source MAC / Transmitter MAC)
  uint8_t addr3[6];       // Address 3 (BSSID)
  uint16_t sequence_ctrl; // Sequence Control (2 bytes) - Sequence number (12 bits) + Fragment number (4 bits)
  uint8_t addr4[6];       // Address 4 (only in WDS) - May not be needed for simple deauth
} mac_hdr_t;

// Structure representing a standard Wi-Fi packet payload following the MAC header
// This is based on common ESP-IDF promiscuous mode usage.
typedef struct {
  mac_hdr_t hdr; // The MAC header member
  uint8_t payload[0]; // Rest of the packet data (flexible array member)
} wifi_packet_t;

// Structure for the raw promiscuous mode packet received from the Wi-Fi driver.
// This structure is typically defined in ESP-IDF headers (like esp_wifi_types.h).
// Ensure this matches the actual structure provided by your ESP-IDF version.
// If `wifi_promiscuous_pkt_t` is already defined in esp_wifi_types.h, do NOT define it here.
// Keeping a potential definition here as a comment for reference:
/*
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl; // Receive control information provided by ESP-IDF
    uint8_t payload[0]; // Raw 802.11 frame data, starting with mac_hdr_t
} wifi_promiscuous_pkt_t;
*/

// Structure for a deauthentication frame (simplified)
typedef struct {
  // Frame Control field (2 bytes) - 0xC000 is the Type/Subtype for Deauthentication
  uint8_t frame_control[2] = { 0xC0, 0x00 };
  uint8_t duration[2];     // Duration field (2 bytes)
  uint8_t station[6];      // Destination MAC Address (the station to deauthenticate)
  uint8_t sender[6];       // Source MAC Address (the AP's MAC, for spoofing)
  uint8_t access_point[6]; // BSSID (the AP's MAC)
  // Fragment number (lower 4 bits) and Sequence number (upper 12 bits)
  // A static value might work for simple deauth, but managing sequence numbers is more standard.
  uint8_t fragment_sequence[2] = { 0x00, 0x00 }; // Initialize to 0, consider managing this
  uint16_t reason;         // Reason Code (2 bytes)
} deauth_frame_t;


// Structure provided by esp_wifi.h for configuring the promiscuous mode filter
// This requires esp_wifi.h to be included BEFORE this definition.
const wifi_promiscuous_filter_t filt = {
  .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA // Filter for Management and Data frames
  // Add other masks if needed, e.g., WIFI_PROMIS_FILTER_MASK_MISC for control frames
};


#endif // TYPE_H
