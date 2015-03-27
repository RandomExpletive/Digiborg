#include "pebble.h"

//dumb ideas:
//shake to display weather info in place of the time, for 3-5 seconds or whatever
//figure out that battery thing
//etc

#define TOTAL_DATE_DIGITS 2
#define TOTAL_TIME_DIGITS 4
#define PM_X 15
#define PM_Y 107
#define PM_W 10
#define PM_H 6
//#define TOTAL_TIME_DIGITS 6 (might not be necessary :D)

static const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_SMALL_SU,
  RESOURCE_ID_IMAGE_DAY_SMALL_MO,
  RESOURCE_ID_IMAGE_DAY_SMALL_TU,
  RESOURCE_ID_IMAGE_DAY_SMALL_WE,
  RESOURCE_ID_IMAGE_DAY_SMALL_TH,
  RESOURCE_ID_IMAGE_DAY_SMALL_FR,
  RESOURCE_ID_IMAGE_DAY_SMALL_SA
};

//repurpose these images for seconds display
static const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};
//re-size complete, further tweaking
//seperators included, perhaps add another 2 pixels on the right side of each :
//definitely blank space between minutes and seconds
static const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_SMALL_0,
  RESOURCE_ID_IMAGE_NUM_SMALL_1,
  RESOURCE_ID_IMAGE_NUM_SMALL_2,
  RESOURCE_ID_IMAGE_NUM_SMALL_3,
  RESOURCE_ID_IMAGE_NUM_SMALL_4,
  RESOURCE_ID_IMAGE_NUM_SMALL_5,
  RESOURCE_ID_IMAGE_NUM_SMALL_6,
  RESOURCE_ID_IMAGE_NUM_SMALL_7,
  RESOURCE_ID_IMAGE_NUM_SMALL_8,
  RESOURCE_ID_IMAGE_NUM_SMALL_9
};

static Window *s_main_window;
static GBitmap *s_meter_bar_bitmap, *s_time_format_bitmap, *s_background_bitmap, *s_day_name_bitmap;
static BitmapLayer *s_background_layer, *s_meter_bar_layer, *s_time_format_layer, *s_day_name_layer;

static GBitmap *s_date_digits[TOTAL_DATE_DIGITS];
static GBitmap *s_time_digits[TOTAL_TIME_DIGITS];
static BitmapLayer *s_date_digits_layers[TOTAL_DATE_DIGITS];
static BitmapLayer *s_time_digits_layers[TOTAL_TIME_DIGITS];

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds = gbitmap_get_bounds((*bmp_image));
#else
  GRect bitmap_bounds = (*bmp_image)->bounds;
#endif
  GRect frame = GRect(origin.x, origin.y, bitmap_bounds.size.w, bitmap_bounds.size.h);
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

  if (old_image != NULL) {
  	gbitmap_destroy(old_image);
  }
}


static unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }

  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12;
}


static void update_display(struct tm *current_time) {
	
	//setup an if statement for:
	//if 24 hour time, use an even smaller set of #s for time
	//else (if 12 hour) use the below
	//or don't because this should be fine, except from 8 PM onward it'll look like ass :D
  set_container_image(&s_day_name_bitmap, s_day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday], GPoint(108, 92));
  set_container_image(&s_date_digits[0], s_date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_sec / 10], GPoint(111, 125));//switched to tm_sec
  set_container_image(&s_date_digits[1], s_date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_sec % 10], GPoint(122, 125));//not currently working

  unsigned short display_hour = get_display_hour(current_time->tm_hour);
  set_container_image(&s_time_digits[0], s_time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour / 10], GPoint(7, 114));//original x = 10
  set_container_image(&s_time_digits[1], s_time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour % 10], GPoint(31, 114));//knocking everything back by 2

  set_container_image(&s_time_digits[2], s_time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min / 10], GPoint(60, 114));//only for now
  set_container_image(&s_time_digits[3], s_time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min % 10], GPoint(86, 114));//comments are fun
	
	//now figure out how to update the seconds every, well, second :D
	
	//hmm... maybe... but probably... idk maybe overlay something... nah thats dumb nevermind

  if (!clock_is_24h_style()) {
    if (current_time->tm_hour >= 12) {
    	layer_set_hidden(bitmap_layer_get_layer(s_time_format_layer), false);
      set_container_image(&s_time_format_bitmap, s_time_format_layer, RESOURCE_ID_IMAGE_PM_SMALL, GPoint(PM_X, PM_Y));//removed magic #s, to change see var on top
    } else {
    	layer_set_hidden(bitmap_layer_get_layer(s_time_format_layer), true);
    }

    if (display_hour / 10 == 0) {
    	layer_set_hidden(bitmap_layer_get_layer(s_time_digits_layers[0]), true);
    } else {
    	layer_set_hidden(bitmap_layer_get_layer(s_time_digits_layers[0]), false);
    }
  }
}

//perhaps change this to handle_second_tick?
/*
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_display(tick_time);
}
*/
static void handle_second_tick(struct tm*tick_time, TimeUnits units_changed)
	{
	update_display(tick_time);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  s_meter_bar_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_METER_BAR);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds = gbitmap_get_bounds(s_meter_bar_bitmap);
#else
  GRect bitmap_bounds = s_meter_bar_bitmap->bounds;
#endif
  GRect frame = GRect(17, 43, bitmap_bounds.size.w, bitmap_bounds.size.h);
  s_meter_bar_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(s_meter_bar_layer, s_meter_bar_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_meter_bar_layer));

  if (!clock_is_24h_style()) {
    s_time_format_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_24_HOUR_MODE);
#ifdef PBL_PLATFORM_BASALT
    bitmap_bounds = gbitmap_get_bounds(s_time_format_bitmap);
#else
    bitmap_bounds = s_time_format_bitmap->bounds;
#endif
    GRect frame = GRect(PM_X, PM_Y, PM_W, PM_H);//got rid of magic #'s, to change x/y of PM display use the var at top
    s_time_format_layer = bitmap_layer_create(frame);
    bitmap_layer_set_bitmap(s_time_format_layer, s_time_format_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_time_format_layer));
  }

  // Create time and date layers
  GRect dummy_frame = GRect(0, 0, 0, 0);
  s_day_name_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_day_name_layer));
  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    s_time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_time_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    s_date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_date_digits_layers[i]));
  }
}

static void main_window_unload(Window *window) {
  layer_remove_from_parent(bitmap_layer_get_layer(s_background_layer));
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);

  layer_remove_from_parent(bitmap_layer_get_layer(s_meter_bar_layer));
  bitmap_layer_destroy(s_meter_bar_layer);
  gbitmap_destroy(s_meter_bar_bitmap);

  layer_remove_from_parent(bitmap_layer_get_layer(s_time_format_layer));
  bitmap_layer_destroy(s_time_format_layer);
  gbitmap_destroy(s_time_format_bitmap);

  layer_remove_from_parent(bitmap_layer_get_layer(s_day_name_layer));
  bitmap_layer_destroy(s_day_name_layer);
  gbitmap_destroy(s_day_name_bitmap);

  for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(s_date_digits_layers[i]));
    gbitmap_destroy(s_date_digits[i]);
    bitmap_layer_destroy(s_date_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(s_time_digits_layers[i]));
    gbitmap_destroy(s_time_digits[i]);
    bitmap_layer_destroy(s_time_digits_layers[i]);
  }
}

static void init() {
  memset(&s_time_digits_layers, 0, sizeof(s_time_digits_layers));
  memset(&s_time_digits, 0, sizeof(s_time_digits));
  memset(&s_date_digits_layers, 0, sizeof(s_date_digits_layers));
  memset(&s_date_digits, 0, sizeof(s_date_digits));

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  update_display(tick_time);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
