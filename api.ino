void API_update_data() {
  HTTPClient https;
  bool new_data = false;

  Serial.println("Getting data update");

  https.begin(client, "https://lite.realtime.nationalrail.co.uk/OpenLDBWS/ldb12.asmx");
  https.addHeader("Content-Type", "text/xml");
  int httpResponseCode = https.POST("<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:typ=\"http://thalesgroup.com/RTTI/2013-11-28/Token/types\" xmlns:ldb=\"http://thalesgroup.com/RTTI/2021-11-01/ldb/\"><soapenv:Header><typ:AccessToken><typ:TokenValue>" + String(NATIONAL_RAIL_TOKEN) + "</typ:TokenValue></typ:AccessToken></soapenv:Header><soapenv:Body><ldb:GetDepartureBoardRequest><ldb:numRows>" + String(NATIONAL_RAIL_ROWS) + "</ldb:numRows><ldb:crs>" + String(NATIONAL_RAIL_CRS) + "</ldb:crs></ldb:GetDepartureBoardRequest></soapenv:Body></soapenv:Envelope>");

  // If successful...
  if (httpResponseCode == 200) {
    
    // Get response and end connection
    String response = https.getString();
    https.end();
    client.stop();

    // Parse XML
    XMLDocument xmlDocument;
    if (xmlDocument.Parse(response.c_str()) != XML_SUCCESS) {
      Serial.println("Error parsing");
      return;
    } else {
      Serial.println("Data received and parsed successfully");
    }

    XMLNode * services = xmlDocument.FirstChildElement("soap:Envelope")->FirstChildElement("soap:Body")->FirstChildElement("GetDepartureBoardResponse")->FirstChildElement("GetStationBoardResult")->FirstChildElement("lt8:trainServices");
    if (services) {
      // Keep count of service processed
      int service_cnt = 0;

      // Process rows (services), should be the same number as requested
      XMLElement * row = services->FirstChildElement();
      while (row != NULL) {
        // Collect the data
        String service_status, service_time, service_platform, service_destination;

        // service_status e.g. On Time, Cancelled, Exp 12:58, ...
        XMLElement * etd = row->FirstChildElement("lt4:etd");
        if (etd) {
          service_status = String(etd->GetText());
          if (service_status != "On time" && service_status != "Delayed" && service_status != "Cancelled" ) {
            service_status = "Exp " + service_status;
          }
        } else {
          service_status = "?";
        }

        // service_time e.g. 12:54
        XMLElement * std = row->FirstChildElement("lt4:std");
        if (std) {
          service_time = String(std->GetText());
        } else {
          service_time = "?";
        }

        // service_platform e.g. Plat 1
        XMLElement * platform = row->FirstChildElement("lt4:platform");
        if (service_platform) {
          service_platform = "Plat " + String(platform->GetText());
        } else {
          service_platform = "Plat ?";
        }

        // service_destination e.g. London Kings Cross
        XMLElement * destination = row->FirstChildElement("lt5:origin")->FirstChildElement("lt4:location")->FirstChildElement("lt4:locationName");
        if (destination) {
          service_destination = String(destination->GetText());
        } else {
          service_destination = "?";
        }

        // Determine whether to update the row, only want to update if there is a change
        if (service_status != departure_service[service_cnt].service_status || service_time != departure_service[service_cnt].service_time || service_platform != departure_service[service_cnt].service_platform || service_destination != departure_service[service_cnt].service_destination) {
          // Fill out row
          if (service_cnt == 0) {
            departure_service[service_cnt].fade = 1;
            departure_service[service_cnt].fade_cnt = 0;
            departure_service[service_cnt].top_left = 0;
            departure_service[service_cnt].x_offset = 0;
            departure_service[service_cnt].hold_cnt = 0;
          } else {
            departure_service[service_cnt].fade = 0;
            departure_service[service_cnt].fade_cnt = 255;
            departure_service[service_cnt].x_offset = 0;
            departure_service[service_cnt].hold_cnt = 0;
            departure_service[service_cnt].top_left = 34;
          }
          departure_service[service_cnt].active = true;
          departure_service[service_cnt].service_number = Ordinal_suffix((1 + service_cnt));
          departure_service[service_cnt].service_status = service_status;
          departure_service[service_cnt].service_time = service_time;
          departure_service[service_cnt].service_platform = service_platform;
          departure_service[service_cnt].service_destination = service_destination;

          // Set to true to force update of matrix
          new_data = true;

          Serial.print("service updated: ");
          Serial.println(departure_service[service_cnt].service_number);
        }

        // Done, move onto next
        row = row->NextSiblingElement();
        service_cnt++;
      }

      // Check if all rows where returned and blank any non-returned rows
      if (service_cnt < NATIONAL_RAIL_ROWS) {
        for (int i = service_cnt; i < NATIONAL_RAIL_ROWS; i += 1) {
          departure_service[i].active = false;
          departure_service[i].top_left = 0;
          departure_service[i].fade = 0;
          departure_service[i].fade_cnt = 255;
          departure_service[i].x_offset = 0;
          departure_service[i].hold_cnt = 0;
        }
      }

      if (new_data) {
        // New data, clear the matrix and reset
        dma_display->clearScreen();
        service_display = 1;
        service_rounds = 0;
        services_total = service_cnt;
      }
    }
  }
}

// Add 1st, 2nd, 3rd suffixes
String Ordinal_suffix(int x) {
  String suffixes[] = {"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"};
  if ((x % 100) >= 11 && (x % 100) <= 13) {
    return "" + String(x) + "th";
  } else {
    return "" + String(x) + "" + suffixes[x % 10] + "";
  }
}
