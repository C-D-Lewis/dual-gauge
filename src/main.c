#include <pebble.h>

#include <pebble-dash-api/pebble-dash-api.h>
#include <pebble-events/pebble-events.h>

#include "windows/main_window.h"

static void init() {
  dash_api_init_appmessage();
  events_app_message_open();
  main_window_push();
}

static void deinit() { }

int main() {
  init();
  app_event_loop();
  deinit();
}