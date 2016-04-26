#include <pebble.h>

#ifndef PBL_ROUND
	#define HEIGHT 168
#else
	#define HEIGHT 180
#endif
#define TIME_LABEL_DISP -12
#define DATE_LABEL_DISP -32
#define NUM_IMG 7

#define KEY_VIBE_TIME 0
#define KEY_VIBE_ON_DISCONECT 1

Window *window;
GBitmap *bitmap;
GBitmap *tb_bitmap;
BitmapLayer *bl;
BitmapLayer *tbl;
TextLayer *timeL;
TextLayer *dateL;
int vibe_time;
bool vibe_on_disconnect;

const uint32_t res[] = {
	RESOURCE_ID_IMG_BLUE_STAR,
	RESOURCE_ID_IMG_MILKEY_WAY,
	RESOURCE_ID_IMG_NEBULA,
	RESOURCE_ID_IMG_STAR_AND_TREE,
	RESOURCE_ID_IMG_STARRY_NIGHT,
	RESOURCE_ID_IMG_SUN,
	RESOURCE_ID_IMG_SWOOSH
};

void deinit();
void init();
static void app_connection_handler(bool connected);
static void kit_connection_handler(bool connected);

static void inbox_received_callback(DictionaryIterator *iterator, void *context){
	Tuple *time = dict_find(iterator, KEY_VIBE_TIME);
	if(time){
		persist_write_int(KEY_VIBE_TIME, time->value->int8);
		vibe_time = time->value->int8;
	}
	
	Tuple *disconnect = dict_find(iterator, KEY_VIBE_ON_DISCONECT);
	if(disconnect){
		persist_write_bool(KEY_VIBE_ON_DISCONECT, disconnect->value->int8);
		vibe_on_disconnect = disconnect->value->int8;
		connection_service_subscribe((ConnectionHandlers) {
  		.pebble_app_connection_handler = app_connection_handler,
  		.pebblekit_connection_handler = kit_connection_handler
		});
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success!!");
}

static void app_connection_handler(bool connected) {
	if (!connected){
		vibes_long_pulse();
	}
  APP_LOG(APP_LOG_LEVEL_INFO, "Pebble app %sconnected", connected ? "" : "dis");
}

static void kit_connection_handler(bool connected) {
  APP_LOG(APP_LOG_LEVEL_INFO, "PebbleKit %sconnected", connected ? "" : "dis"); 
}

void set_time(){
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	static char t_buffer[8];
	strftime(t_buffer, sizeof(t_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
	
	text_layer_set_text(timeL, t_buffer);
	
	static char d_buffer[25];
	strftime(d_buffer, sizeof(d_buffer), "%b %d", tick_time);
	
	text_layer_set_text(dateL, d_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
	set_time();
	
	if(tick_time->tm_min+(tick_time->tm_hour) % 60 == 0 || tick_time->tm_min+(tick_time->tm_hour) == 0){
		window_stack_pop(false);
		deinit();
		init();
	}
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", vibe_time);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", tick_time->tm_min+(tick_time->tm_hour) % vibe_time);
	
	if(vibe_time != 0 && (tick_time->tm_min+(tick_time->tm_hour)) % vibe_time == 0){
		vibes_double_pulse();
	}
	
}

void main_window_load(){
	Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
	
	int random = ((double)rand()/RAND_MAX)*NUM_IMG;
	
	bitmap = gbitmap_create_with_resource(res[random]);
	bl = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(bl, bitmap);
	
	tb_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMG_TIME_BACKGROUND);
	tbl = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(tbl, tb_bitmap);
	bitmap_layer_set_background_color(tbl, GColorClear);
	bitmap_layer_set_compositing_mode(tbl, GCompOpSet);
	
	
	GSize size = graphics_text_layout_get_content_size("00:00", fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STAR_TREK_38)),
																										 bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
	
	timeL = text_layer_create(GRect(0, HEIGHT/2 + TIME_LABEL_DISP, bounds.size.w, size.h));
	text_layer_set_font(timeL, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STAR_TREK_38)));
	text_layer_set_background_color(timeL, GColorClear);
	text_layer_set_text_alignment(timeL, GTextAlignmentCenter);
	text_layer_set_text_color(timeL, GColorWhite);
	text_layer_set_text(timeL, "10:20");
	
	GSize size2 = graphics_text_layout_get_content_size("Wed", fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STAR_TREK_28)),
																										 bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
	
	dateL = text_layer_create(GRect(0, HEIGHT/2+DATE_LABEL_DISP, bounds.size.w, size2.h));
	text_layer_set_font(dateL, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STAR_TREK_28)));
	text_layer_set_background_color(dateL, GColorClear);
	text_layer_set_text_alignment(dateL, GTextAlignmentCenter);
	text_layer_set_text_color(dateL, GColorWhite);
	text_layer_set_text(dateL, "APR 25");
	
	layer_add_child(window_layer, bitmap_layer_get_layer(bl));
	layer_add_child(window_layer, bitmap_layer_get_layer(tbl));
	layer_add_child(window_layer, text_layer_get_layer(timeL));
	layer_add_child(window_layer, text_layer_get_layer(dateL));
	
	set_time();
}

void main_window_unload(){
	bitmap_layer_destroy(bl);
	bitmap_layer_destroy(tbl);
	gbitmap_destroy(bitmap);
	gbitmap_destroy(tb_bitmap);
	text_layer_destroy(timeL);
	text_layer_destroy(dateL);
}

void init(){
	vibe_time = 0;
	if(persist_exists(KEY_VIBE_TIME)){
		vibe_time = persist_read_int(KEY_VIBE_TIME);
	}
	
	vibe_on_disconnect = false;
	if(persist_exists(KEY_VIBE_ON_DISCONECT)){
		vibe_on_disconnect = persist_read_bool(KEY_VIBE_ON_DISCONECT);
	}
	
	window = window_create();
	
	window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
	
	window_stack_push(window, true);
	
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	if(vibe_on_disconnect){
		connection_service_subscribe((ConnectionHandlers) {
  		.pebble_app_connection_handler = app_connection_handler,
  		.pebblekit_connection_handler = kit_connection_handler
		});
	}
	
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
}

void deinit(){
	window_destroy(window);
}

int main(){
	init();
	app_event_loop();
	deinit();
}