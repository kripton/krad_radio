#include "krad_ipc_server.h"
#include "hardlimiter.h"
#include "sidechain_comp.h"
#include "djeq.h"
#include "digilogue.h"
#include "pass.h"
#include "fastlimiter.h"

#include <jack/jack.h>

#define PORTGROUP_MAX 20

// #include "kiss_fft.h"

typedef struct krad_mixer_client_St krad_mixer_client_t;
typedef struct krad_mixer_St krad_mixer_t;

typedef enum {
	OUTPUT,
	INPUT,
} portgroup_direction_t;

typedef enum {
	NIL,
	MONO,
	STEREO,
	THREE,
	QUAD,
	FIVE,
	SIX,
} portgroup_channel_t;

typedef enum {
	MIC,
	MUSIC,
	AUX,
	MAINMIX,
	SUBMIX,
	STEREOMIC
} portgroup_type_t;

typedef struct portgroup_St portgroup_t;

struct portgroup_St {
	char name[256];
	portgroup_direction_t direction;
	portgroup_type_t type;
	portgroup_channel_t channels;
	int group;
	jack_port_t *input_port;
	jack_port_t *input_ports[8];
	jack_port_t *output_ports[8];
	float volume[8];
	
	float final_volume_actual[8];
	float final_fade_actual[8];
	float music_fade_value[8];
	float aux_fade_value[8];
	
	
	float volume_actual[8];
	float new_volume_actual[8];
	int last_sign[8];
	float peak[8];
	jack_default_audio_sample_t *samples[8];
	int active;
	int monitor;
	
	int djeq_on;
	djeq_t *djeq;
	fastlimiter_t *fastlimiter;
	
	krad_mixer_t *krad_mixer;
	
	portgroup_t *next;
};


struct krad_mixer_St {

	krad_ipc_server_t *ipc;
	
	char station[256];
	
	const char **ports;
	const char *client_name;
	const char *server_name;
	jack_options_t options;
	jack_status_t status;
	jack_client_t *jack_client;

	int shutdown;
	int xruns;
	int sample_rate;
		
    void *userdata;
    
//    krad_mixer_client_t *client;
	portgroup_t *portgroup[PORTGROUP_MAX];

	// this means group of portgroups aka a mic group daah
	int portgroup_count;

	sc2_data_t *sc2_data;
	sc2_data_t *sc2_data2;
	
	//digilogue_t *digilogue_left;
	//digilogue_t *digilogue_right;
	
	//pass_t *pass_left;
	//pass_t *pass_right;
	
	//djeq_t *djeq;
	float music_1_2_crossfade;
	float music_aux_crossfade;
	
	float new_music1_fade_value[8];
	float new_music2_fade_value[8];
	float new_music_fade_value[8];
	float new_aux_fade_value[8];
	
	float music1_fade_value[8];
	float music2_fade_value[8];
	
	float fade_temp;
	float fade_temp_pos;	
	
	int active_input;
	int last_active_input;
	int active_input_bytes;
	char active_input_data[2048];
	pthread_t active_input_thread;
	
	jack_default_audio_sample_t micmix_left[1024];
	jack_default_audio_sample_t micmix_right[1024];

	int level_bytes;
	char level_data[2048];
	float level_float_value;
	int level_pg;
	portgroup_t *level_portgroup;
	
	pthread_t levels_thread;
	
};


struct krad_mixer_client_St {

	char data[2048];
	krad_mixer_t *krad_mixer;
	
	char *cmd;
	char datacpy[512];
	char *temp;

	char *unit;
	char *subunit;
	char *control;
	
	char path[128];
	char path2[128];
	char path3[128];
	char path4[128];
	char path5[128];
	char path6[128];
	float float_value;

	float fade_temp;
	float fade_temp_pos;
	float volume_temp;

	int pos;	
	int cnt1;
	int cnt2;
	char portname[256];
	int pg;
	portgroup_t *portgroup;
};

void example_session(krad_mixer_t *krad_mixer);
krad_mixer_t *krad_mixer_setup(char *jack_name_suffix);

float read_stereo_peak(portgroup_t *portgroup);
int xrun_callback(void *arg);
void *krad_mixer_new_client(void *arg);
void krad_mixer_close_client(void *arg);
void krad_mixer_shutdown(void *arg);
void krad_mixer_want_shutdown();
int krad_mixer_handler(char *data, void *pointer);
int setcontrol(krad_mixer_client_t *client, char *data);
void compute_peak(portgroup_t *portgroup, int channel, int sample_count);
void compute_peaks(portgroup_t *portgroup, int sample_count);
void listcontrols(krad_mixer_client_t *client, char *data);

