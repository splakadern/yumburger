// Header File: definitions.h
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// --- Serial Debugging ---
#define SERIAL_DEBUG // Uncomment this line to enable serial debug output
#ifdef SERIAL_DEBUG
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) // Define as nothing if debug is off
#endif

// --- LED Configuration ---
// Define the LED pin number if you have an LED connected
// #define LED 2 // Example: Built-in LED on some ESP32 boards

#ifdef LED
#define BLINK_LED(times, duration) blink_led(times, duration)
// Declare the blink_led function if LED is defined (you'll need to define it in your main .ino)
extern void blink_led(int num_times, int blink_duration);
#else
#define BLINK_LED(times, duration) // Define as nothing if LED is off
#endif

// --- Wi-Fi Configuration ---
// SoftAP credentials for the web interface
#define AP_SSID "ESP32-Deauther"
#define AP_PASS "deauther" // Minimum 8 characters for WPA2

// Max channel for 2.4GHz channel hopping (1-13 typically, 14 restricted)
#define CHANNEL_MAX 13 // Standard WiFi channels up to 13 for 2.4GHz

// --- Deauth Parameters ---
// Number of deauth frames to send per identified station (Placeholder - implementation needed)
#define NUM_FRAMES_PER_DEAUTH 10 // Example value

// Deauth attack types
#define DEAUTH_TYPE_NONE    0 // No deauth attack active
#define DEAUTH_TYPE_SINGLE  1 // Deauth a single selected network
#define DEAUTH_TYPE_ALL     2 // Deauth all networks (channel hopping)

// LED Blink parameters for deauth activity (if LED is defined - Placeholder)
#define DEAUTH_BLINK_TIMES      1 // How many times to blink per deauth burst
#define DEAUTH_BLINK_DURATION   50 // Total duration of the blink sequence (ms)

#endif // DEFINITIONS_H
