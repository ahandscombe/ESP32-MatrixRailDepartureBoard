#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "src/tinyxml2/tinyxml2.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
MatrixPanel_I2S_DMA *dma_display = nullptr;
#include "src/font/departure_board_font.h"
using namespace tinyxml2;

#define WIFI_SSID "network_name"
#define WIFI_PASSWORD "ABC123"
#define NATIONAL_RAIL_TOKEN "example_token" // National Rail Darwin Token
#define NATIONAL_RAIL_CRS "BFR" // Station code: https://www.nationalrail.co.uk/stations_destinations/48541.aspx
#define NATIONAL_RAIL_ROWS 6 // How many services should be displayed?
#define UPDATE_INTERVAL 30000 // How many milliseconds between API updates?

#define PANEL_RES_X 64 // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 64 // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1  // Total number of panels chained one to another

struct departure_struct {
  bool active =  false;
  byte top_left = 0;
  byte fade = 0; // 0 = Static or no fade, 1 = fade in, 2 = fade out
  byte fade_cnt = 255; // Used for fading in or out to keep status
  int x_offset = 0;
  int hold_cnt = 0;
  String service_number = "";
  String service_status = "";
  String service_time = "";
  String service_platform = "";
  String service_destination = "";
};
departure_struct departure_service[NATIONAL_RAIL_ROWS];

unsigned long api_previous_check = -50000; // Force update immediately
unsigned long matrix_update_timer = 0;
int services_total = 0; // Total number of services returned
int service_rounds = 0; // Keep count of how many loops a row has been shown
int service_display = 1; // service_display, which service to display in the bottom row

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);

  // Matrix setup
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // module width
    PANEL_RES_Y,   // module height
    PANEL_CHAIN    // Chain length
  );

  mxconfig.gpio.e = 18;
  mxconfig.clkphase = false;

  // Set start state of matrix
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(60);
  dma_display->clearScreen();

  // SSL setup
  client.setInsecure();

  // Start background loop
  xTaskCreatePinnedToCore(Task_background, "Task_background", 20000, NULL, 2, NULL, 0);

  // Start matrix handling loop
  xTaskCreatePinnedToCore(Task_matrix, "Task_matrix", 20000, NULL, 2, NULL, 1);
}

void loop() {
  // Empty. Things are done in the Tasks below
}

void Task_background(void *pvParameters) {
  // Do background things here like network things or updating API data
  Network_setup();

  for (;;) // A Task shall never return or exit.
  {
    Network_loop();

    // Update API data
    if (Network_wifi_check() == true) {
      unsigned long currentMillis = millis();
      if ((currentMillis - api_previous_check) >= UPDATE_INTERVAL) {
        api_previous_check = currentMillis;
        API_update_data();
      }
    }
    vTaskDelay(10);  // delay in between reads for stability
  }
}

void Task_matrix(void *pvParameters) {
  // Update the matrix here

  // Wait until wifi is connected
  while (Network_wifi_check() == false) {
    vTaskDelay(10);
  }

  dma_display->clearScreen(); // Clear the display once more
  vTaskDelay(100); // Wait a bit more

  for (;;) // A Task shall never return or exit.
  {
    // Update display
    Matrix_display();
    vTaskDelay(10);  // delay in between reads for stability
  }
}
