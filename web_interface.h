// Header File: web_interface.h
#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WebServer.h> // For WebServer class
#include "wifi_manager.h" // Needed to call wifi_perform_scan and getEncryptionType

// Declare the WebServer object (defined in your main .ino)
extern WebServer server;
// Declare the num_networks variable (defined in your main .ino)
extern int num_networks;


// --- Function Declarations ---

// Initializes and starts the web interface
void start_web_interface();

// Handles incoming client requests for the web server
void web_interface_handle_client();

// Web server request handlers (declared here, defined in web_interface.cpp)
void handle_root();
void handle_deauth(); // Handles requests to start single AP deauth
void handle_deauth_all(); // Handles requests to start deauth all
void handle_rescan(); // Handles requests to rescan networks
void handle_stop(); // Handles requests to stop the deauth attack

// Helper function for web redirects
void redirect_root();

#endif // WEB_INTERFACE_H
