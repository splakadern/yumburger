// Source File: wifi_manager.cpp

#include "wifi_manager.h"

// External declarations for variables defined in the main .ino
extern WebServer server; // The web server instance
extern int num_networks; // Stores the number of scanned networks

// Initializes Wi-Fi for the web interface (SoftAP)
void wifi_init_ap() {
    DEBUG_PRINTF("Initializing WiFi in AP mode...\n");
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    DEBUG_PRINTF("SoftAP started. IP address: %s\n", WiFi.softAPIP().toString().c_str());
}

// Initializes Wi-Fi for scanning or deauth (STA)
void wifi_init_sta() {
    DEBUG_PRINTF("Initializing WiFi in STA mode...\n");
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(true); // Disconnect from any previous connection
    delay(100); // Give it a moment
}

// Performs a synchronous Wi-Fi scan
void wifi_perform_scan() {
    DEBUG_PRINTF("Starting WiFi scan...\n");
    num_networks = WiFi.scanNetworks();
    DEBUG_PRINTF("Scan complete. Found %d networks.\n", num_networks);
}

// Gets the encryption type as a string for web UI
String getEncryptionType(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
    // WPA3 modes might not be defined in all ESP-IDF versions/configs
    // case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
    // case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
    case WIFI_AUTH_MAX: // This is usually the last enum value
    default: return "UNKNOWN"; // Handle any other cases
  }
}
