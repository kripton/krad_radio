#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>  
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "krad_system.h"

#define PRO_REEL_SIZE 26.67
#define PRO_REEL_SPEED 38.1
#define NORMAL_REEL_SIZE 17.78
#define NORMAL_REEL_SPEED 19.05
#define SLOW_NORMAL_REEL_SPEED 9.53

// device colors
#define BLUE 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0
#define BLUE_TRANS 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0, 0.255
#define BLUE_TRANS2 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0, 0.144 / 0.255 * 1.0
#define BLUE_TRANS3 0.0, 0.122 / 0.255 * 1.0, 0.112 / 0.255 * 1.0, 0.144 / 0.255 * 1.0
#define GREEN  0.001 / 0.255 * 1.0, 0.187 / 0.255 * 1.0, 0.0
#define LGREEN  0.001 / 0.255 * 1.0, 0.187 / 0.255 * 1.0, 0.0, 0.044 / 0.255 * 1.0
#define WHITE 0.222 / 0.255 * 1.0, 0.232 / 0.255 * 1.0, 0.233 / 0.255 * 1.0
#define WHITE_TRANS 0.222 / 0.255 * 1.0, 0.232 / 0.255 * 1.0, 0.233 / 0.255 * 1.0, 0.555
#define ORANGE  0.255 / 0.255 * 1.0, 0.080 / 0.255 * 1.0, 0.0
#define GREY  0.197 / 0.255 * 1.0, 0.203 / 0.255 * 1.0, 0.203 / 0.255   * 1.0
#define GREY2  0.037 / 0.255 * 1.0, 0.037 / 0.255 * 1.0, 0.038 / 0.255   * 1.0
#define BGCOLOR  0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.033 / 0.255   * 1.0
#define GREY3  0.103 / 0.255 * 1.0, 0.103 / 0.255 * 1.0, 0.124 / 0.255   * 1.0

#define BGCOLOR_TRANS  0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.144 / 0.255 * 1.0

#define BGCOLOR_CLR  0.0 / 0.255 * 1.0, 0.0 / 0.255 * 1.0, 0.0 / 0.255   * 1.0, 0.255 / 0.255   * 1.0
// ui colors
// green 79d789
// dark bluegreen 22404f
// middle pink dc3b5b
// e1e1e1 whitegrey
// purple 5d71cb
// dark grey 575768

typedef enum {
	REEL_TO_REEL,
	PLAYBACK_STATE_STATUS,
} krad_gui_item_t;

typedef enum {
	krad_gui_STOPPED,
	krad_gui_PAUSED,
	krad_gui_PLAYING,
} krad_gui_playback_state_t;


typedef struct krad_gui_St krad_gui_t;
typedef struct krad_gui_reel_to_reel_St krad_gui_reel_to_reel_t;
typedef struct krad_gui_playback_state_status_St krad_gui_playback_state_status_t;

//typedef struct krad_gui_recording_status_St krad_gui_recording_status_t;

struct krad_gui_St {

	int stride;
	int bytes;
	unsigned char *pixels;
	cairo_surface_t *cst;
	int external_surface;
	int internal_surface;

	cairo_t *cr;
	int width;
	int height;
	int size;
	unsigned long long frame;
	
	int shutdown;

	char current_track_time_timecode_string[512];
	char total_track_time_timecode_string[512];
	char elapsed_time_timecode_string[512];
	char playback_state_status_string[512];
	char debugline[512];
    
	struct timespec current_time;
    
	struct timespec start_time;
	struct timespec elapsed_time;
	
	struct timespec total_track_time;
	struct timespec current_track_time;
	
	float current_track_progress;
	unsigned int current_track_time_ms;
	unsigned int total_track_time_ms;
	unsigned int elapsed_time_ms;

	krad_gui_playback_state_t playback_state;

	krad_gui_reel_to_reel_t *reel_to_reel;
	krad_gui_playback_state_status_t *playback_state_status;
	
	void *gui_ptr;
	
	void (*control_speed_callback)(void *, float);
	void (*control_speed_down_callback)(void *);
	void (*control_speed_up_callback)(void *);
	void *callback_pointer;
	
	int overlay;
	
	int render_timecode;
	
	//krad_gui_recording_status_t *recording_status;
	
	int render_rgb;
	
	int recording;
	struct timespec recording_start_time;
	struct timespec recording_elapsed_time;
	char recording_time_timecode_string[512];
	
	int recording_box_width;
	int recording_box_height;
	int recording_box_margin;
	int recording_box_padding;
	int recording_box_font_size;
	
	int live;
	struct timespec live_start_time;
	struct timespec live_elapsed_time;
	char live_time_timecode_string[512];
	
	int live_box_width;
	int live_box_height;
	int live_box_margin;
	int live_box_padding;
	int live_box_font_size;
	
	int render_test_text;
	char test_text[512];
	char test_text_time[512];
	char test_start_time[512];
	char test_info_text[512];
	
	int render_rotator;
	int rotator_angle;
	int rotator_speed;
	
	
	int render_tearbar;
	float tearbar_speed;
	float tearbar_positioner;
	int tearbar_position;
	float tearbar_speed_adj;
	int tearbar_width;
	int movement_range;
	int last_tearbar_position;
	int new_speed;
	
	int render_wheel;
	int wheel_angle;
	
	int render_ftest;
	
	int update_drawtime;
	int print_drawtime;
	char draw_time_string[192];
	struct timespec draw_time;
	struct timespec start_draw_time;
	struct timespec end_draw_time;

	float input_val[8];
	float output_val[8];
	float input_peak[8];
	float output_peak[8];
	float input_current[8];
	float output_current[8];


	int cursor_x;
	int cursor_y;
	int click;
	int mousein;
	
	int clear;
	
	cairo_surface_t *bug;
	int bug_height;
	int bug_width;
	float bug_fade;
	
	float bug_alpha;
	
	int bug_x;
	int bug_y;
	int render_bug;
	char *next_bug;
	float bug_fade_speed;
	float bug_fader;
	//int start_frame;

	
};

struct krad_gui_reel_to_reel_St {

	krad_gui_t *krad_gui;
	
	float total_distance;
	float distance;
	float angle;
	
	float reel_size;
	
	// At nominal playback speed
	float reel_speed;

};

struct krad_gui_playback_state_status_St {

	krad_gui_t *krad_gui;

};

int krad_gui_render_selector_selected (krad_gui_t *krad_gui, int x, int y, int size, char *label);

void krad_gui_set_click (krad_gui_t *krad_gui, int click);
void krad_gui_set_pointer (krad_gui_t *krad_gui, int x, int y);

void krad_gui_set_surface (krad_gui_t *krad_gui, cairo_surface_t *cst);

void krad_gui_render_selector (krad_gui_t *krad_gui, int x, int y, int w);

void krad_gui_render_hex (krad_gui_t *krad_gui, int x, int y, int w);
void krad_gui_set_bug (krad_gui_t *krad_gui, char *filename, int x, int y);
void krad_gui_remove_bug (krad_gui_t *krad_gui);
void krad_gui_load_bug (krad_gui_t *krad_gui, char *filename);

void krad_gui_start_draw_time(krad_gui_t *krad_gui);
void krad_gui_end_draw_time(krad_gui_t *krad_gui);

void krad_gui_control_speed_up(krad_gui_t *krad_gui);
void krad_gui_control_speed_down(krad_gui_t *krad_gui);
void krad_gui_control_speed(krad_gui_t *krad_gui, float value);
void krad_gui_set_control_speed_callback(krad_gui_t *krad_gui, void control_speed_callback(void *, float));
void krad_gui_set_control_speed_down_callback(krad_gui_t *krad_gui, void control_speed_down_callback(void *));
void krad_gui_set_control_speed_up_callback(krad_gui_t *krad_gui, void control_speed_up_callback(void *));
void krad_gui_set_callback_pointer(krad_gui_t *krad_gui, void *callback_pointer);

void krad_gui_render_vtest (krad_gui_t *krad_gui);

krad_gui_t *krad_gui_create_with_external_surface (int width, int height, unsigned char *pixels);

void krad_gui_create_external_surface(krad_gui_t *krad_gui);
void krad_gui_destroy_external_surface(krad_gui_t *krad_gui);


void krad_gui_clear (krad_gui_t *krad_gui);
void krad_gui_overlay_clear (krad_gui_t *krad_gui);

void krad_gui_create_internal_surface(krad_gui_t *krad_gui);
void krad_gui_destroy_internal_surface(krad_gui_t *krad_gui);
krad_gui_t *krad_gui_create_with_internal_surface(int width, int height);
krad_gui_t *krad_gui_create(int width, int height);
void krad_gui_destroy(krad_gui_t *krad_gui);
void krad_gui_render(krad_gui_t *krad_gui);
void krad_gui_set_size(krad_gui_t *krad_gui, int width, int height);
void krad_gui_set_background_color(krad_gui_t *krad_gui, float r, float g, float b, float a);
void krad_gui_add_item(krad_gui_t *krad_gui, krad_gui_item_t item);
void krad_gui_remove_item(krad_gui_t *krad_gui, krad_gui_item_t item);

void krad_gui_test_screen(krad_gui_t *krad_gui, char *info);
void krad_gui_render_rgb(krad_gui_t *krad_gui);
void krad_gui_render_live(krad_gui_t *krad_gui);
void krad_gui_render_recording(krad_gui_t *krad_gui);
void krad_gui_render_rotator(krad_gui_t *krad_gui);
void krad_gui_test_text(krad_gui_t *krad_gui);

void krad_gui_render_text (krad_gui_t *krad_gui, int x, int y, int size, char *string);

void krad_gui_start_recording(krad_gui_t *krad_gui);
void krad_gui_stop_recording(krad_gui_t *krad_gui);

void krad_gui_go_live(krad_gui_t *krad_gui);
void krad_gui_go_off(krad_gui_t *krad_gui);
void krad_gui_render_cube (krad_gui_t *krad_gui, int x, int y, int w, int h);
void krad_gui_render_tearbar(krad_gui_t *krad_gui);
void krad_gui_render_wheel(krad_gui_t *krad_gui);
void krad_gui_render_ftest(krad_gui_t *krad_gui);

void krad_gui_render_meter (krad_gui_t *krad_gui, int x, int y, int size, float pos);

krad_gui_reel_to_reel_t *krad_gui_reel_to_reel_create(krad_gui_t *krad_gui);
void krad_gui_reel_to_reel_destroy(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel);
void krad_gui_render_reel(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel, int x, int y);
void krad_gui_render_reel_to_reel(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel);
void krad_gui_update_reel_to_reel_information(krad_gui_reel_to_reel_t *krad_gui_reel_to_reel);

krad_gui_playback_state_status_t *krad_gui_playback_state_status_create(krad_gui_t *krad_gui);
void krad_gui_playback_state_status_destroy(krad_gui_playback_state_status_t *krad_gui_playback_state_status);
void krad_gui_render_playback_state_status(krad_gui_playback_state_status_t *krad_gui_playback_state_status);

void krad_gui_set_playback_state(krad_gui_t *krad_gui, krad_gui_playback_state_t playback_state);
void krad_gui_add_current_track_time_ms(krad_gui_t *krad_gui, unsigned int additional_ms);
void krad_gui_update_current_track_time_timecode_string(krad_gui_t *krad_gui);
void krad_gui_update_total_track_time_timecode_string(krad_gui_t *krad_gui);
void krad_gui_update_elapsed_time_timecode_string(krad_gui_t *krad_gui);
void krad_gui_update_playback_state_status_string(krad_gui_t *krad_gui);

void krad_gui_update_current_track_time_ms(krad_gui_t *krad_gui);
void krad_gui_update_total_track_time_ms(krad_gui_t *krad_gui);
void krad_gui_update_elapsed_time_ms(krad_gui_t *krad_gui);

void krad_gui_update_current_track_progress(krad_gui_t *krad_gui);

void krad_gui_reset_elapsed_time(krad_gui_t *krad_gui);
void krad_gui_update_elapsed_time(krad_gui_t *krad_gui);
void krad_gui_set_current_track_time(krad_gui_t *krad_gui, struct timespec current_track_time);
void krad_gui_set_current_track_time_ms(krad_gui_t *krad_gui, unsigned int elapsed_time_ms);
void krad_gui_set_total_track_time(krad_gui_t *krad_gui, struct timespec total_track_time);
void krad_gui_set_total_track_time_ms(krad_gui_t *krad_gui, unsigned int total_track_time_ms);

struct timespec timespec_diff(struct timespec start, struct timespec end);
