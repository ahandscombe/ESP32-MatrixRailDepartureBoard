// Custom Wifi Management
// Sometimes the ESP32 likes to disconnect from Wifi. So keep checking for connection.
unsigned long previousCheck = 0;

void Network_setup() {
  // Wifi
  Network_wifi_setup();
  Network_wifi_connection(true);
}

void Network_loop() {
  Network_wifi_connection(false);
}

void Network_wifi_setup() {
  WiFi.onEvent(Network_wifi_event_disconnect, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

void Network_wifi_connection(bool force) {
  unsigned long currentMillis = millis();

  // Every twenty seconds check if Wifi is connected
  if (force == true || (currentMillis - previousCheck) >= 20000) {
    // Are we connected?
    if (!Network_wifi_check()) {
      // Disconnect and reconnect to network
      WiFi.disconnect(true);
      WiFi.persistent(false);
      WiFi.setAutoConnect(false);
      WiFi.setAutoReconnect(false);
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }

    previousCheck = currentMillis;
  }
}

// Event: disconnect
void Network_wifi_event_disconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  Network_wifi_connection(true);
}

// Does the device have a working wifi connection
bool Network_wifi_check() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  } else {
    return true;
  }
}
