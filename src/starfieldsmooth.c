#include <pebble.h>

#define CORNER_RADIUS 1
#define CHROME 0
#define MAX_STARS 60

static Window *window;
static Layer *canvas;
static TextLayer *time_layer;
static AppTimer *timer;
static uint32_t delta = 33;
static int visible_stars = 0;
static char time_text[] = "00:00";

struct Star {
	short x;
	short y;
	short radius;
	bool visible;
};

struct Star *stars[MAX_STARS];

static void applog(const char* message)
{
	app_log(APP_LOG_LEVEL_INFO, "renderer.c", 0, message);
}

/********************************** Star Lifecycle *****************************/

static struct Star* create_star()
{
	struct Star *this = malloc(sizeof(struct Star));
	this->x = 0;
	this->y = 0;
	this->radius = 1;
	this->visible = false;
	
	return this;
}

static void prepare_stars()
{ 
	for(int i = 0; i < MAX_STARS; i++)
	{
		stars[i] = create_star();
	} 
}

static void destroy_stars()
{
	for(int i = 0; i < MAX_STARS; i++)
	{
		free(stars[i]);
	}
}
	
/****************************** Renderer Lifecycle *****************************/

static void update()
{
	//Spawn (one per frame)
	if(visible_stars < MAX_STARS)
	{
		//Find next slot
		int i = 0;
		for(i = 0; i < MAX_STARS; i++)
		{
			if(stars[i]->visible == false)
			{
				break;	//Found a slot
			}
		}
		
		if(i < MAX_STARS)
		{		
			//Spawn one
			stars[i]->visible = true;
			stars[i]->x = 0;
			stars[i]->y = rand() % 168;	//Between 0 and 168
			stars[i]->radius = rand() % 5;
			
			if(stars[i]->radius < 1)
			{
				stars[i]->radius++;
			}
		
			visible_stars++;
		}
	}
	
	//Update all existing
	for(int i = 0; i < MAX_STARS; i++)
	{
		//Evaluate visibility
		if(stars[i]->x >= 144 + stars[i]->radius)
		{
			stars[i]->visible = false;	//Dissapears for recycling
			visible_stars--;
		}
		
		//Update visible
		if(stars[i]->visible == true)
		{
			//Proportional speed
			stars[i]->x += stars[i]->radius;
		}
	}
}

/*
 * Render re-scheduling
 */
static void timer_callback(void *data) {	
	//Do logic
	update();

	//Render
	layer_mark_dirty(canvas);
	
	//Register next render
	timer = app_timer_register(delta, (AppTimerCallback) timer_callback, 0);
}

/*
 * Start rendering loop
 */
static void start()
{
	timer = app_timer_register(delta, (AppTimerCallback) timer_callback, 0);
}

/*
 * Rendering
 */
static void render(Layer *layer, GContext* ctx) 
{
	//Setup
  graphics_context_set_fill_color(ctx, GColorWhite);

	//Draw Star.radius rects for all stars
	for(int i = 0; i < MAX_STARS; i++)
	{
		if(stars[i]->visible == true)
		{
			graphics_fill_rect(ctx, GRect(stars[i]->x, stars[i]->y, stars[i]->radius, stars[i]->radius), 0, GCornerNone);
		}
	}
}

/****************************** Window Lifecycle *****************************/

static void set_time_display(struct tm *t)
{
	int size = sizeof("00:00");

	if(clock_is_24h_style())
	{
		strftime(time_text, size, "%H:%M", t);
	}
	else
	{
		strftime(time_text, size, "%I:%M", t);
	}
	
	text_layer_set_text(time_layer, time_text);
}

static void window_load(Window *window)
{
	//Setup window
	window_set_background_color(window, GColorBlack);
	
	//Setup canvas
	canvas = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(canvas, (LayerUpdateProc) render);
	layer_add_child(window_get_root_layer(window), canvas);
	
	//Setup text layer
	time_layer = text_layer_create(GRect(0, 60, 144, 50));
	text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_38)));
	text_layer_set_text_color(time_layer, GColorWhite);
	text_layer_set_background_color(time_layer, GColorClear);
	text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
	
	//Set initial time so display isn't blank
	struct tm *t;
	time_t temp;	
	temp = time(NULL);	
	t = localtime(&temp);
	
	//Works to here
	set_time_display(t);
	
	//Start rendering
	start();
}

static void window_unload(Window *window) 
{
	//Cancel timer
	app_timer_cancel(timer);
	
	//Destroy canvas
	layer_destroy(canvas);
	
	//Destroy time layer
	text_layer_destroy(time_layer);
}

/****************************** App Lifecycle *****************************/

static void tick_handler(struct tm *t, TimeUnits units_changed)
{
	set_time_display(t);
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) 
	{
		.load = window_load,
		.unload = window_unload,
	});
	
	//Prepare stars memory
	prepare_stars();
	
	//Tick tock
	tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick_handler);
	
	//Finally
	window_stack_push(window, true);
}

static void deinit(void) 
{
	//De-init stars
	destroy_stars();
	
	//No more ticks
	tick_timer_service_unsubscribe();

	//Finally
	window_destroy(window);
}

int main(void) 
{
	init();
	app_event_loop();
	deinit();
}
