// Header File: wifi_manager.h
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "definitions.h" // For AP_SSID, AP_PASS

// Declare the WebServer object (defined in your main .ino)
extern WebServer server;
// Declare the num_networks variable (defined in your main .ino)
extern int num_networks;

// --- Function Declarations ---

// Initializes Wi-Fi for the web interface (SoftAP)
void wifi_init_ap();

// Initializes Wi-Fi for scanning or deauth (STA)
void wifi_init_sta();

// Performs a synchronous Wi-Fi scan
void wifi_perform_scan();

// Gets the encryption type as a string for web UI
String getEncryptionType(wifi_auth_mode_t encryptionType);

#endif // WIFI_MANAGER_H
