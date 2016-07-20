#include <pebble.h>

#include <pebble-dash-api/pebble-dash-api.h>
#include <pebble-events/pebble-events.h>

#define APP_NAME "Dual Gauge"
#define NO_VALUE -1

static Window *s_window;
static TextLayer *s_time_layer;
static Layer *s_canvas_layer;

static GBitmap *s_watch_bitmap, *s_phone_bitmap;
static int s_local_perc, s_remote_perc = NO_VALUE;

static void error_callback(ErrorCode code) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "ErrorCode: %d", code);

  s_remote_perc = NO_VALUE;
  layer_mark_dirty(s_canvas_layer);
}

static void get_data_callback(DataType type, DataValue value) {
  s_remote_perc = value.integer_value;
  layer_mark_dirty(s_canvas_layer);
}

static void update_gauges() {
  BatteryChargeState state = battery_state_service_peek();
  s_local_perc = (int)state.charge_percent;

  dash_api_get_data(DataTypeBatteryPercent, get_data_callback);
  layer_mark_dirty(s_canvas_layer);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  const int top = 120;
  const int bottom = 0;
  const int lr = PBL_IF_ROUND_ELSE(50, 30);
  GRect text_bounds = grect_inset(bounds, GEdgeInsets(top, lr, bottom, lr));
  static char s_local_buff[8];
  snprintf(s_local_buff, sizeof(s_local_buff), "%d", s_local_perc);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_local_buff, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS), 
    text_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  static char s_remote_buff[8];
  snprintf(s_remote_buff, sizeof(s_remote_buff), (s_remote_perc == NO_VALUE) ? "-" : "%d", s_remote_perc);
  graphics_draw_text(ctx, s_remote_buff, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS), 
    text_bounds, GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  GRect bitmap_rect = GRect(PBL_IF_ROUND_ELSE(43, 24), 80, 40, 40);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_watch_bitmap, bitmap_rect);

  bitmap_rect.origin.x += PBL_IF_ROUND_ELSE(56, 57);
  graphics_draw_bitmap_in_rect(ctx, s_phone_bitmap, bitmap_rect);  
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  static char s_buff[8];
  strftime(s_buff, sizeof(s_buff), "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_buff);

  if(tick_time->tm_min % 15 == 0) {
    update_gauges();
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_watch_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_WATCH);
  s_phone_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_PHONE);

  GSize time_size = GSize(120, 45);
  const int tb = (bounds.size.h - time_size.h) / 4;
  const int lr = (bounds.size.w - time_size.w) / 2;
  s_time_layer = text_layer_create(grect_inset(bounds, GEdgeInsets(tb, lr)));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  update_gauges();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  layer_destroy(s_canvas_layer);

  gbitmap_destroy(s_watch_bitmap);
  gbitmap_destroy(s_phone_bitmap);

  window_destroy(s_window);
}

void main_window_push() {
  dash_api_init(APP_NAME, error_callback);
  events_app_message_open();

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  events_tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  time_t now = time(NULL);
  struct tm *time_now = localtime(&now);
  tick_handler(time_now, MINUTE_UNIT);
}
