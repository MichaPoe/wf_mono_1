#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "resource_ids.auto.h"

#include "string.h"
#include "stdlib.h"

#include "wf_mono_1.h"

#define MY_UUID { 0x0C, 0xAF, 0x2A, 0xED, 0x77, 0xAC, 0x41, 0x69, 0x99, 0x2B, 0x89, 0x78, 0x9C, 0x13, 0x1F, 0xD3 }

/*
	History
	0.1 initial version
 */

PBL_APP_INFO(MY_UUID,
	"Mono #1",
	"MichaPoe",
	0, 1, /* App version */
	RESOURCE_ID_IMAGE_MENU_ICON,
	APP_INFO_WATCH_FACE);

static const char *GERMAN_DAYS[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };

static struct SimpleAnalogData {
  Layer simple_bg_layer;
  Layer date_layer;
  TextLayer day_label;
  char day_buffer[3];
  TextLayer num_label;
  char num_buffer[3];
  Layer time_layer;
  TextLayer time_label;
  char time_buffer[6];
  Layer week_layer;
  TextLayer week_label;
  char week_buffer[6];

  GPath hour_arrow;
  GPath tick_paths[NUM_CLOCK_TICKS];
  Layer hands_layer;
  Window window;
} s_data;

static void bg_update_proc(Layer* me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, me->bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_draw_filled(ctx, &s_data.tick_paths[i]);
  }
}

static void hands_update_proc(Layer* me, GContext* ctx) {
  // const GPoint center = grect_center_point(&me->bounds);

  PblTm t;
  get_time(&t);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(&s_data.hour_arrow, (TRIG_MAX_ANGLE * (((t.tm_hour % 12) * 6) + (t.tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, &s_data.hour_arrow);
  gpath_draw_outline(ctx, &s_data.hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(me->bounds.size.w / 2, me->bounds.size.h / 2), 6);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, GPoint(me->bounds.size.w / 2, me->bounds.size.h / 2), 3);
}

static void date_update_proc(Layer* me, GContext* ctx) {
  (void) me;
  (void) ctx;

  PblTm t;
  get_time(&t);

  text_layer_set_text(&s_data.day_label, GERMAN_DAYS[t.tm_wday]);
  //string_format_time(s_data.day_buffer, sizeof(s_data.day_buffer), "%a", &t);
  //text_layer_set_text(&s_data.day_label, s_data.day_buffer);

  string_format_time(s_data.num_buffer, sizeof(s_data.num_buffer), "%d", &t);
  text_layer_set_text(&s_data.num_label, s_data.num_buffer);
}

static void time_update_proc(Layer* me, GContext* ctx) {
  (void) me;
  (void) ctx;

  PblTm t;
  get_time(&t);

#ifdef SHOWONLYMINUTES
	#define TIMEFORMAT "%M"
#else
	#define TIMEFORMAT "%M:%H"
#endif
  string_format_time(s_data.time_buffer, sizeof(s_data.time_buffer), TIMEFORMAT, &t);
  text_layer_set_text(&s_data.num_label, s_data.num_buffer);
}

static void week_update_proc(Layer* me, GContext* ctx) {
  (void) me;
  (void) ctx;

  PblTm t;
  get_time(&t);

  string_format_time(s_data.week_buffer, sizeof(s_data.week_buffer), "KW %V", &t);
  text_layer_set_text(&s_data.num_label, s_data.num_buffer);
}

static void handle_init(AppContextRef app_ctx) {
  window_init(&s_data.window, "MichaPoe mono #1");
  window_stack_push(&s_data.window, true);
  resource_init_current_app(&WF_MONO_1);

  s_data.day_buffer[0] = '\0';
  s_data.num_buffer[0] = '\0';
  s_data.time_buffer[0] = '\0';
  s_data.week_buffer[0] = '\0';

  // Fonts
  GFont font_normal = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  GFont font_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  // init hand paths
  gpath_init(&s_data.hour_arrow, &HOUR_HAND_POINTS);

  const GPoint center = grect_center_point(&s_data.window.layer.bounds);
  gpath_move_to(&s_data.hour_arrow, center);

  // init clock face paths
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_init(&s_data.tick_paths[i], &ANALOG_BG_POINTS[i]);
  }

  // init layers
  layer_init(&s_data.simple_bg_layer, s_data.window.layer.frame);
  s_data.simple_bg_layer.update_proc = &bg_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.simple_bg_layer);

  // resolution: 144 × 168

  // init date layer -> a plain parent layer to create a date update proc
  layer_init(&s_data.date_layer, s_data.window.layer.frame);
  s_data.date_layer.update_proc = &date_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.date_layer);

  // init day
  text_layer_init(&s_data.day_label, GRect(95, 68, 25, 24));
  text_layer_set_text(&s_data.day_label, s_data.day_buffer);
  text_layer_set_background_color(&s_data.day_label, GColorBlack);
  text_layer_set_text_color(&s_data.day_label, GColorWhite);
  text_layer_set_font(&s_data.day_label, font_normal);
  layer_add_child(&s_data.date_layer, &s_data.day_label.layer);

  // init num
  text_layer_init(&s_data.num_label, GRect(125, 68, 25, 24));
  text_layer_set_text_alignment(&s_data.day_label, GTextAlignmentRight);
  text_layer_set_text(&s_data.num_label, s_data.num_buffer);
  text_layer_set_background_color(&s_data.num_label, GColorBlack);
  text_layer_set_text_color(&s_data.num_label, GColorWhite);
  text_layer_set_font(&s_data.num_label, font_bold);
  layer_add_child(&s_data.date_layer, &s_data.num_label.layer);

  // time layer -> update proc
  layer_init(&s_data.time_layer, s_data.window.layer.frame);
  s_data.time_layer.update_proc = &time_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.time_layer);

  // init time
  text_layer_init(&s_data.time_label, GRect(0, 68, 45, 24));
  text_layer_set_text(&s_data.time_label, s_data.time_buffer);
  text_layer_set_background_color(&s_data.time_label, GColorBlack);
  text_layer_set_text_color(&s_data.time_label, GColorWhite);
  text_layer_set_font(&s_data.time_label, font_normal);
  layer_add_child(&s_data.time_layer, &s_data.time_label.layer);

  // week layer -> update proc
  layer_init(&s_data.week_layer, s_data.window.layer.frame);
  s_data.week_layer.update_proc = &week_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.week_layer);

  // init week
  text_layer_init(&s_data.week_label, GRect(50, 144, 44, 24));
  text_layer_set_text_alignment(&s_data.week_label, GTextAlignmentCenter);
  text_layer_set_text(&s_data.week_label, s_data.week_buffer);
  text_layer_set_background_color(&s_data.week_label, GColorBlack);
  text_layer_set_text_color(&s_data.week_label, GColorWhite);
  text_layer_set_font(&s_data.week_label, font_normal);
  layer_add_child(&s_data.week_layer, &s_data.week_label.layer);

  // init hands
  layer_init(&s_data.hands_layer, s_data.simple_bg_layer.frame);
  s_data.hands_layer.update_proc = &hands_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.hands_layer);
}

static void handle_minute_tick(AppContextRef ctx, PebbleTickEvent* t) {
  (void) t;
  layer_mark_dirty(&s_data.window.layer);
}

void pbl_main(void* params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}

