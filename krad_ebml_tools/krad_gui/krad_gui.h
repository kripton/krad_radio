#include <cairo.h>
#include <gtk/gtk.h>
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

#define PRO_REEL_SIZE 26.67
#define PRO_REEL_SPEED 38.1
#define NORMAL_REEL_SIZE 17.78
#define NORMAL_REEL_SPEED 19.05
#define SLOW_NORMAL_REEL_SPEED 9.53

// device colors
#define BLUE 0.0, 0.152 / 0.255 * 1.0, 0.212 / 0.255 * 1.0
#define GREEN  0.001 / 0.255 * 1.0, 0.187 / 0.255 * 1.0, 0.0
#define WHITE 0.222 / 0.255 * 1.0, 0.232 / 0.255 * 1.0, 0.233 / 0.255 * 1.0
#define WHITE_TRANS 0.222 / 0.255 * 1.0, 0.232 / 0.255 * 1.0, 0.233 / 0.255 * 1.0, 0.555
#define ORANGE  0.255 / 0.255 * 1.0, 0.080 / 0.255 * 1.0, 0.0
#define GREY  0.197 / 0.255 * 1.0, 0.203 / 0.255 * 1.0, 0.203 / 0.255   * 1.0
#define GREY2  0.037 / 0.255 * 1.0, 0.037 / 0.255 * 1.0, 0.038 / 0.255   * 1.0
#define BGCOLOR  0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.033 / 0.255   * 1.0
#define GREY3  0.103 / 0.255 * 1.0, 0.103 / 0.255 * 1.0, 0.124 / 0.255   * 1.0

#define BGCOLOR_TRANS  0.033 / 0.255 * 1.0, 0.033 / 0.255 * 1.0, 0.033 / 0.255   * 1.0, 0.144 / 0.255 * 1.0

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
} kradgui_item_t;

typedef enum {
	KRADGUI_STOPPED,
	KRADGUI_PAUSED,
	KRADGUI_PLAYING,
} kradgui_playback_state_t;


typedef struct kradgui_St kradgui_t;
typedef struct kradgui_reel_to_reel_St kradgui_reel_to_reel_t;
typedef struct kradgui_playback_state_status_St kradgui_playback_state_status_t;

//typedef struct kradgui_recording_status_St kradgui_recording_status_t;

struct kradgui_St {

	int stride;
	int bytes;
	unsigned char *data;
	cairo_surface_t *cst;
	int internal_surface;

	cairo_t *cr;
	int width;
	int height;
	int size;
	unsigned long long frame;

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

	kradgui_playback_state_t playback_state;

	kradgui_reel_to_reel_t *reel_to_reel;
	kradgui_playback_state_status_t *playback_state_status;
	
	void *gui_ptr;
	
	void (*control_speed_callback)(void *, float);
	void (*control_speed_down_callback)(void *);
	void (*control_speed_up_callback)(void *);
	void *callback_pointer;
	
	int overlay;
	
	int render_timecode;
	
	//kradgui_recording_status_t *recording_status;
	
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
	
};

struct kradgui_reel_to_reel_St {

	kradgui_t *kradgui;
	
	float total_distance;
	float distance;
	float angle;
	
	float reel_size;
	
	// At nominal playback speed
	float reel_speed;

};

struct kradgui_playback_state_status_St {

	kradgui_t *kradgui;

};

void kradgui_start_draw_time(kradgui_t *kradgui);
void kradgui_end_draw_time(kradgui_t *kradgui);

void kradgui_control_speed_up(kradgui_t *kradgui);
void kradgui_control_speed_down(kradgui_t *kradgui);
void kradgui_control_speed(kradgui_t *kradgui, float value);
void kradgui_set_control_speed_callback(kradgui_t *kradgui, void control_speed_callback(void *, float));
void kradgui_set_control_speed_down_callback(kradgui_t *kradgui, void control_speed_down_callback(void *));
void kradgui_set_control_speed_up_callback(kradgui_t *kradgui, void control_speed_up_callback(void *));
void kradgui_set_callback_pointer(kradgui_t *kradgui, void *callback_pointer);


void kradgui_create_internal_surface(kradgui_t *kradgui);
void kradgui_destroy_internal_surface(kradgui_t *kradgui);
kradgui_t *kradgui_create_with_internal_surface(int width, int height);
kradgui_t *kradgui_create(int width, int height);
void kradgui_destroy(kradgui_t *kradgui);
void kradgui_render(kradgui_t *kradgui);
void kradgui_set_size(kradgui_t *kradgui, int width, int height);
void kradgui_set_background_color(kradgui_t *kradgui, float r, float g, float b, float a);
void kradgui_add_item(kradgui_t *kradgui, kradgui_item_t item);
void kradgui_remove_item(kradgui_t *kradgui, kradgui_item_t item);

void kradgui_test_screen(kradgui_t *kradgui, char *info);
void kradgui_render_rgb(kradgui_t *kradgui);
void kradgui_render_live(kradgui_t *kradgui);
void kradgui_render_recording(kradgui_t *kradgui);
void kradgui_render_rotator(kradgui_t *kradgui);
void kradgui_test_text(kradgui_t *kradgui);

void kradgui_start_recording(kradgui_t *kradgui);
void kradgui_stop_recording(kradgui_t *kradgui);

void kradgui_go_live(kradgui_t *kradgui);
void kradgui_go_off(kradgui_t *kradgui);

void kradgui_render_tearbar(kradgui_t *kradgui);
void kradgui_render_wheel(kradgui_t *kradgui);
void kradgui_render_ftest(kradgui_t *kradgui);

kradgui_reel_to_reel_t *kradgui_reel_to_reel_create(kradgui_t *kradgui);
void kradgui_reel_to_reel_destroy(kradgui_reel_to_reel_t *kradgui_reel_to_reel);
void kradgui_render_reel(kradgui_reel_to_reel_t *kradgui_reel_to_reel, int x, int y);
void kradgui_render_reel_to_reel(kradgui_reel_to_reel_t *kradgui_reel_to_reel);
void kradgui_update_reel_to_reel_information(kradgui_reel_to_reel_t *kradgui_reel_to_reel);

kradgui_playback_state_status_t *kradgui_playback_state_status_create(kradgui_t *kradgui);
void kradgui_playback_state_status_destroy(kradgui_playback_state_status_t *kradgui_playback_state_status);
void kradgui_render_playback_state_status(kradgui_playback_state_status_t *kradgui_playback_state_status);

void kradgui_set_playback_state(kradgui_t *kradgui, kradgui_playback_state_t playback_state);
void kradgui_add_current_track_time_ms(kradgui_t *kradgui, unsigned int additional_ms);
void kradgui_update_current_track_time_timecode_string(kradgui_t *kradgui);
void kradgui_update_total_track_time_timecode_string(kradgui_t *kradgui);
void kradgui_update_elapsed_time_timecode_string(kradgui_t *kradgui);
void kradgui_update_playback_state_status_string(kradgui_t *kradgui);

void kradgui_update_current_track_time_ms(kradgui_t *kradgui);
void kradgui_update_total_track_time_ms(kradgui_t *kradgui);
void kradgui_update_elapsed_time_ms(kradgui_t *kradgui);

void kradgui_update_current_track_progress(kradgui_t *kradgui);

void kradgui_reset_elapsed_time(kradgui_t *kradgui);
void kradgui_update_elapsed_time(kradgui_t *kradgui);
void kradgui_set_current_track_time(kradgui_t *kradgui, struct timespec current_track_time);
void kradgui_set_current_track_time_ms(kradgui_t *kradgui, unsigned int elapsed_time_ms);
void kradgui_set_total_track_time(kradgui_t *kradgui, struct timespec total_track_time);
void kradgui_set_total_track_time_ms(kradgui_t *kradgui, unsigned int total_track_time_ms);

struct timespec timespec_diff(struct timespec start, struct timespec end);
