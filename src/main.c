#include <pebble.h>

#define MAX_STARS 27
#define DELTA 50

typedef struct {
  short x;
  short y;
  short radius;
  bool visible;
  bool planet;
  GColor color;
} Star;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static Layer *s_canvas;

static GSize s_screen_size;
static AppTimer *s_render_timer;
static Star *s_stars[MAX_STARS];

static int s_visible_stars = 0;

GColor planet_colors[5] = {GColorRed, GColorRajah, GColorBrightGreen, GColorBlueMoon, GColorShockingPink};
GColor time_colors[5] = {GColorVividCerulean, GColorElectricBlue, GColorMediumSpringGreen, GColorYellow, GColorRichBrilliantLavender};

/********************************** Star Lifecycle ****************************/

static Star* star_create() {
  Star *this = (Star*)malloc(sizeof(Star));
  this->x = 0;
  this->y = 0;
  this->radius = 1;
  this->visible = false;
  this->planet = false;
  this->color = GColorWhite;
  return this;
}

static void star_destroy(Star *this) {
	free(this);
}

static void stars_init() { 
  for(int i = 0; i < MAX_STARS; i++) {
    s_stars[i] = star_create();
  } 
}

static void stars_deinit() {
  for(int i = 0; i < MAX_STARS; i++) {
    star_destroy(s_stars[i]);
  }
}
  
/****************************** Renderer Lifecycle ****************************/

static void update() {
	// Spawn a new star?
  if(s_visible_stars < MAX_STARS) {
    // Find next slot
    int i = 0;
    for(i = 0; i < MAX_STARS; i++) {
      if(!s_stars[i]->visible) {
      	// Found a free slot
        break;  
      }
    }
    
    if(i < MAX_STARS) {    
      // Spawn one and set up its properties
      s_stars[i]->visible = true;
      s_stars[i]->x = 0;
      s_stars[i]->y = rand() % s_screen_size.h;  // Between 0 and s_screen_size.h
      s_stars[i]->radius = rand() % 7;
      if(s_stars[i]->radius >= 6 && rand() % 4 == 2) {
        s_stars[i]->radius = 8 + rand() % 4;
        s_stars[i]->planet = true;
        s_stars[i]->color = planet_colors[rand() % 4];
      }
      
      if(s_stars[i]->radius < 1) {
        s_stars[i]->radius++;
      }
      s_visible_stars++;
    }
  }
  
  // Update all existing s_stars
  for(int i = 0; i < MAX_STARS; i++) {
    // Evaluate visibility
    if(s_stars[i]->x >= s_screen_size.w + s_stars[i]->radius) {
      s_stars[i]->visible = false;  // Dissapears for recycling
      s_stars[i]->planet = false;
      s_stars[i]->color = GColorWhite;
      s_visible_stars--;
    }
    
    // Update visible
    if(s_stars[i]->visible) {
      // Proportional speed
      s_stars[i]->x += s_stars[i]->radius;
    }
  }
}

static void next_frame(void *context) {  
  // Do logic
  update();

  // Render
  layer_mark_dirty(s_canvas);
  
  // Register next render
  s_render_timer = app_timer_register(DELTA, next_frame, 0);
}

static void start_rendering() {
  s_render_timer = app_timer_register(DELTA, next_frame, 0);
}

static void render_update_proc(Layer *layer, GContext *ctx) {
  // Draw Star.radius rects for all s_stars
  for(int i = 0; i < MAX_STARS; i++) {
    if(s_stars[i]->visible) {
      graphics_context_set_fill_color(ctx, s_stars[i]->color);
      graphics_fill_rect(ctx, GRect(s_stars[i]->x, s_stars[i]->y, s_stars[i]->radius, s_stars[i]->radius), 0, GCornerNone);
    }
  }
}

/****************************** Window Lifecycle *****************************/

char *upcase(char *str) {
  for (int i = 0; str[i] != 0; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 0x20;
    }
  }

  return str;
}

static void set_time_display(struct tm *t) {
  int col = rand() % 5;
  text_layer_set_text_color(s_time_layer, time_colors[col]);
  text_layer_set_text_color(s_date_layer, time_colors[col]);

	static char s_time_buffer[8];
  static char s_date_buffer[16];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H\r\n%M" : "%I\r\n%M", t);
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %e", t);
  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, upcase(s_date_buffer));
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

  s_screen_size.w = bounds.size.w;
  s_screen_size.h = bounds.size.h;

  // Setup s_canvas Layer
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, render_update_proc);
  layer_add_child(window_layer, s_canvas);

  // Setup time TextLayer
  s_date_layer = text_layer_create(GRect(1, 12, bounds.size.w-12, 30));
  text_layer_set_font(s_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AERX_18)));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_time_layer = text_layer_create(GRect(1, bounds.size.h-61, 60, 60));
  text_layer_set_font(s_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GAMEPLAY_24)));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Set initial time so display isn't blank
  time_t temp = time(NULL);  
  struct tm *t = localtime(&temp);
  set_time_display(t);
  
  start_rendering();
}

static void window_unload(Window *window) {
  app_timer_cancel(s_render_timer);
  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_canvas);
}

/****************************** App Lifecycle *****************************/

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  set_time_display(tick_time);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  stars_init();

  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick_handler);
  window_stack_push(s_main_window, true);
}

static void deinit() {
  stars_deinit();

  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
