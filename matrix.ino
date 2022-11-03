void Matrix_display() {
  unsigned long currentMillis = millis();

  // Update matrix every 130ms
  if (currentMillis - matrix_update_timer >= 130) {
    matrix_update_timer = currentMillis;

    for (int i = 0; i < (sizeof(departure_service) / sizeof(departure_service[0])); i += 1) {
      if (departure_service[i].active == false && service_display == i) {
        // Skip and increment counter
        service_display++;
        continue;
      }

      // Is this service is being displayed?
      if (service_display == i) {
        if (service_rounds == 0) {
          // Start fading In
          departure_service[i].fade = 1;
          departure_service[i].fade_cnt = 0;
          service_rounds++;
        } else if (service_rounds > 16 && service_rounds < 100) {
          // Ensure full brightness
          departure_service[i].fade = 0;
          departure_service[i].fade_cnt = 255;
          service_rounds++;
        } else if (service_rounds == 100) {
          // Start fading out only if there is more than two rows
          if (services_total > 2) {
            departure_service[i].fade = 2;
            departure_service[i].fade_cnt = 255;
            service_rounds++;
          }
        } else if ( service_rounds == 117) {
          // Display the next service, will only happen if there is more than two rows
          service_display++;
          service_rounds = 0;
          // Reset scroll position
          departure_service[i].x_offset = 0;
          departure_service[i].hold_cnt = 0;
        } else {
          // Just increment counter
          service_rounds++;
        }
      }

      if (i == 0 || service_display == i) {
        // Print the row
        Matrix_departure_row(
          departure_service[i].top_left,
          &departure_service[i].fade,
          &departure_service[i].fade_cnt,
          &departure_service[i].x_offset,
          &departure_service[i].hold_cnt,
          departure_service[i].service_number,
          departure_service[i].service_status,
          departure_service[i].service_time,
          departure_service[i].service_platform,
          departure_service[i].service_destination
        );
      }
    }

    // Reset service_display
    if (service_display >= (sizeof(departure_service) / sizeof(departure_service[0]))) {
      service_display = 1;
    }
  }
}

void Matrix_departure_row(byte top_left, byte* fade, byte* fade_cnt, int* x_offset, int* hold_cnt, String service_number, String service_status, String service_time, String service_platform, String service_destination) {
  // top_left = y pixelId co-ordinate which points to top-left of row
  // fade = 0 = Static or no fade, 1 = fade in, 2 = fade out
  // fade_cnt = 255 is full brightness and at 0 all text is black
  // x_offset = Used by the function to record the scroll position
  // hold_cnt = Used by the function to record hold status before scrolling
  // service_number = 1st, 2nd, 3rd, ...
  // service_status = On Time, Exp 12:55, Cancelled, ...
  // service_time = 12:54, ...
  // service_platform = Plat 1, Plat 2, ...
  // service_destination = London Kings Cross, Peterborough, ...

  // Bounds Calcs
  int16_t  x1, y1;
  uint16_t w, h;

  byte color_white[3] = {255, 255, 255};
  byte color_blue[3] = {83, 90, 255};
  byte color_yellow[3] = {187, 168, 0};
  if (*fade != 0) {
    // Fading is happening
    color_white[0] = *fade_cnt;
    color_white[1] = *fade_cnt;
    color_white[2] = *fade_cnt;
    color_blue[0] = map(*fade_cnt, 0, 255, 0, 83);
    color_blue[1] = map(*fade_cnt, 0, 255, 0, 90);
    color_blue[2] = *fade_cnt;
    color_yellow[0] = map(*fade_cnt, 0, 255, 0, 187);
    color_yellow[1] = map(*fade_cnt, 0, 255, 0, 168);
    color_yellow[2] = 0;

    if (*fade == 1) {
      // Fade in
      if ( *fade_cnt != 255) {
        *fade_cnt += 15;
      }
    } else {
      // Fade Out
      if ( *fade_cnt != 0) {
        *fade_cnt -= 15;
      }
    }
  } else {
    *fade_cnt = 255;
  }

  dma_display->setFont(&departure_board_font);
  dma_display->setTextWrap(false);
  dma_display->setTextSize(1); // 1x size multiplier

  // service_number
  dma_display->setCursor(0, (top_left + 6));
  dma_display->setTextColor(dma_display->color565(color_blue[0], color_blue[1], color_blue[2]));
  dma_display->println(service_number);

  // service_status
  dma_display->setTextColor(dma_display->color565(color_yellow[0], color_yellow[1], color_yellow[2]));
  dma_display->getTextBounds(service_status, 0, 0, &x1, &y1, &w, &h);
  dma_display->setCursor((dma_display->width() - w), (top_left + 6));
  dma_display->println(service_status);

  // service_time
  dma_display->setTextColor(dma_display->color565(color_white[0], color_white[1], color_white[2]));
  dma_display->setCursor(0, (top_left + 16));
  dma_display->println(service_time);

  // service_platform
  dma_display->setTextColor(dma_display->color565(color_white[0], color_white[1], color_white[2]));
  dma_display->getTextBounds(service_platform, 0, 0, &x1, &y1, &w, &h);
  dma_display->setCursor((dma_display->width() - w), (top_left + 16));
  dma_display->println(service_platform);

  // destination
  dma_display->setTextColor(dma_display->color565(color_white[0], color_white[1], color_white[2]));
  dma_display->getTextBounds(service_destination, 0, 0, &x1, &y1, &w, &h);

  if (w < dma_display->width()) {
    // Less than width of matrix so don't scroll
    dma_display->setCursor(0, (top_left + 26));
    dma_display->println(service_destination);
  } else {
    // Scroll, same as or wider than the matrix
    dma_display->setCursor(*x_offset, (top_left + 26));
    dma_display->fillRect(0, (top_left + 20), dma_display->width(), h, dma_display->color444(0, 0, 0));
    dma_display->println(service_destination);

    // Hold the start position for 10 frames
    if (*hold_cnt > 10) {
      *x_offset -= 1;
    } else {
      *hold_cnt += 1;
    }

    // Reset when the text has scrolled off the matrix
    if (*x_offset == (0 - (w + 1))) {
      *x_offset = 0;
      *hold_cnt = 0;
    }
  }
}
