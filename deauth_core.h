// Header File: deauth_core.h
#ifndef DEAUTH_CORE_H
#define DEAUTH_CORE_H

#include <Arduino.h>
#include <esp_wifi.h> // For wifi_promiscuous_pkt_type_t and wifi_promiscuous_filter_t
#include "type.h" // For deauth_frame_t and mac_hdr_t
#include "definitions.h" // For DEAUTH_TYPE_ macros

// --- Function Declarations ---

// Starts a deauthentication attack
// network_num: The index of the target network from the scan results (used in single mode)
// type: The type of attack (DEAUTH_TYPE_SINGLE or DEAUTH_TYPE_ALL)
// reason_code: The reason code to include in the deauth frame
void start_deauth(int network_num, int type, uint16_t reason_code);

// Stops the currently active deauthentication attack
void stop_deauth();

// --- External Declarations ---
// Declare the sniffer function defined in the main .ino so deauth_core.cpp can use it to set the callback
extern IRAM_ATTR void sniffer(void *buf, wifi_promiscuous_pkt_type_t type);

// Declare the filter defined in type.h
extern const wifi_promiscuous_filter_t filt;

// Declare global variables from the main .ino so deauth_core.cpp can access them
extern deauth_frame_t deauth_frame;
extern int deauth_type;
extern int eliminated_stations;
extern int curr_channel;

#endif // DEAUTH_CORE_H
