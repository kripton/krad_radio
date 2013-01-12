#ifndef KRAD_CLIENT_H
#define KRAD_CLIENT_H


/**
 * @file krad_radio_client.h
 * @brief Krad Radio Client API
 */

/**
 * @mainpage Krad Radio Client API
 *
 * These documents contain all types and functions needed to to 
 * IPC conmmunication to manage, control and modify krad radio stations.
 * 
 * Sections:
 * @li @ref krad_radio_client_ctl
 * @li @ref krad_radio_client
 * @li @ref krad_mixer_client
 * @li @ref krad_compositor_client
 * @li @ref krad_transponder_client
 *
 */


#include <inttypes.h>


#define ALL_BROADCASTS 1


/** @defgroup krad_radio_client Krad Radio Client API
 * @brief Get and Manage general client functions like IPC-connection, 
 * IPC-responses, system information, uptime, enable or disbale remote 
 * control and list / manage tags for one station
 * @{
 */

/** @name General Functions
 * General functions for connection handling
 * @{
 */

/** Krad Radio Client connection handle.
  * @brief Type to identify a single IPC-connection to a station
  * @see kr_connect,kr_disconnect
  */
typedef struct kr_client_St kr_client_t;

/** Shared memory buffer.
 * @brief Variable sized buffer used to get data in and out of local A/V ports
 * @see kr_shm_create,kr_shm_destroy
 */
typedef struct kr_shm_St kr_shm_t;


typedef struct kr_response_St kr_response_t;

typedef enum {
  KR_FLOAT,
	KR_INT32,
	KR_STRING,
} kr_subunit_control_data_t;

typedef union {
  int integer;
  char *string;
  float real;
} kr_subunit_control_value_t;

typedef enum {
  KR_RADIO,
	KR_MIXER,
	KR_COMPOSITOR,
	KR_TRANSPONDER,
} kr_unit_t;

typedef enum {
	KR_PORTGROUP,
//	PORTGROUP_EFFECT,
} kr_mixer_subunit_t;

typedef enum {
	KR_VOLUME,
	KR_CROSSFADE,
} kr_mixer_portgroup_control_t;

/*
typedef enum {
	EQ_DB,
	EQ_BANDWIDTH,
	EQ_HZ,	
	PASS_TYPE,
	PASS_BANDWIDTH,
	PASS_HZ,
} kr_mixer_portgroup_effect_control_t;
*/

typedef enum {
	KR_VIDEOPORT,
	KR_SPRITE,
	KR_TEXT,
	KR_VECTOR,
} kr_compositor_subunit_t;

typedef enum {
	KR_X,
	KR_Y,
	KR_Z,
	KR_WIDTH,
	KR_HEIGHT,
	KR_ROTATION,
	KR_OPACITY,
	KR_XSCALE,
	KR_YSCALE,
	KR_RED,
	KR_GREEN,
	KR_BLUE,
	KR_ALPHA,
} kr_compositor_control_t;

typedef enum {
	KR_TRANSMITTER,
	KR_RECEIVER,
	KR_DEMUXER,
	KR_MUXER,
	KR_ENCODER,
	KR_DECODER,
} kr_transponder_subunit_t;

typedef enum {
	KR_BUFFER,
	KR_BITRATE,
} kr_transponder_control_t;

typedef struct kr_unit_control_St kr_unit_control_t;
typedef struct kr_subunit_control_path_St kr_subunit_control_path_t;

typedef union {
  kr_mixer_subunit_t mixer_subunit;
  kr_compositor_subunit_t compositor_subunit;
  kr_transponder_subunit_t transponder_subunit;
} kr_subunit_t;

typedef union {
  kr_mixer_portgroup_control_t portgroup_control;
//  kr_mixer_portgroup_effect_control_t portgroup_effect_control;
  kr_compositor_control_t compositor_control;
  kr_transponder_control_t transponder_control;
} kr_subunit_control_t;

struct kr_subunit_control_path_St {
  kr_unit_t unit;
  kr_subunit_t subunit;
  kr_subunit_control_t control;
};

typedef union {
  int subunit_number;
  char subunit_name[64];
} kr_subunit_address_t;

struct kr_unit_control_St {
  kr_subunit_control_path_t path;
  kr_subunit_address_t address;
  kr_subunit_control_data_t data_type;
  kr_subunit_control_value_t value;
  int duration;
  //krad_ease_t easing;
};

int kr_subunit_control_set (kr_client_t *kr_client,
                            kr_unit_t unit,
                            kr_subunit_t subunit,
                            kr_subunit_address_t address,
                            kr_subunit_control_t control,
                            kr_subunit_control_value_t value);



typedef int (*item_callback_t)( unsigned char *, uint64_t, char ** );

kr_client_t *kr_client_create (char *client_name);

/**
 * @brief connect to a krad radio daemon identified by sysname
 * @param sysname of local station or ip:port remote station
 * @return connection handle or NULL on error
 */
int kr_connect (kr_client_t *client, char *sysname);


int kr_connect_remote (kr_client_t *client, char *host, int port);


int kr_connected (kr_client_t *client);

/**
 * @brief disconnect an open IPC-connection
 * @param kr_client pointer to handle of the connection to be closed
 */
int kr_disconnect (kr_client_t *client);


int kr_client_destroy (kr_client_t **client);

/**
 * @brief determines if a connection is local or remote
 * @param kr_client handle of the IPC-connection to the station
 * @return 1 if local, 0 otherwise
 */
int kr_client_local (kr_client_t *kr_client);

/**
 * @brief subscribe to broadcast messages of a specific type on one station
 * @param kr_client handle of the IPC-connection to the station
 * @param broadcast_id type of the broadcast messages
 * @see kr_poll
 */
void kr_broadcast_subscribe (kr_client_t *kr_client, uint32_t broadcast_id);

/**
 * @brief check for new broadcast messages after having subscribed to them
 * @param kr_client handle of the IPC-connection to the station
 * @param timeout_ms timeout on checking
 * @return > 0 if there are new messages, 0 otherwise
 * @see kr_broadcast_subscribe
 */
int kr_poll (kr_client_t *kr_client, uint32_t timeout_ms);



void kr_response_free (kr_response_t **kr_response);
int kr_response_to_string (kr_response_t *kr_response, char **string);
int kr_response_to_int (kr_response_t *kr_response, int *number);
void kr_response_free_string (char **string);
int kr_response_get_string (unsigned char *ebml_frag, uint64_t ebml_data_size, char **string);

int kr_response_is_list (kr_response_t *kr_response);
int kr_response_list_length (kr_response_t *kr_response);

kr_unit_t kr_response_unit (kr_response_t *kr_response);
uint32_t kr_response_size (kr_response_t *kr_response);

void kr_client_response_wait_print (kr_client_t *kr_client);


/**
 * @brief get a response
 * @param kr_client handle of the IPC-connection to the station
 */
void kr_client_response_get (kr_client_t *kr_client, kr_response_t **kr_response);

/**
 * @brief waits for a response
 * @param kr_client handle of the IPC-connection to the station
 */
void kr_client_response_wait (kr_client_t *kr_client, kr_response_t **kr_response);


/** @} */

/** @name Shared Memory Functions
 * Functions for managing the shared memory buffer for local A/V ports
 * @{
 */

/**
 * @brief creates and allocates a shared memory buffer
 * @param client handle of the IPC-connection to the station
 * @return handle for the shared memory buffer, NULL on error
 * @todo currently, this size is fixed for resolution 960x540. this needs to be changed!
 */
kr_shm_t *kr_shm_create (kr_client_t *client);

/**
 * @brief destroys and frees an allocated shared memory buffer
 * @param kr_shm handle of the buffer to be destroyed
 */
void kr_shm_destroy (kr_shm_t *kr_shm);

/** @} */

/** @name Information Functions
 * Functions for to query information from a station
 * @{
 */

/**
 * @brief Prints out the uptime of the station
 * @param client handle of the IPC-connection to the station
 */
void kr_uptime (kr_client_t *client);

/**
 * @brief Prints out system information (hostname, architecture and kernel 
 * version) from where the station is running on
 * @param client handle of the IPC-connection to the station
 */
void kr_system_info (kr_client_t *client);

/**
 * @brief Prints out the current CPU usage of the station
 * @param client handle of the IPC-connection to the station
 */
void kr_system_cpu_usage (kr_client_t *client);

/**
 * @brief Sets the "working directory" for the station where logfiles and 
 * snaphots are stored. The directory must exist, it will not be created 
 * for you!
 * @param client handle of the IPC-connection to the station
 * @param dir
 */
void kr_set_dir (kr_client_t *client, char *dir);

/**
 * @brief Prints out the currently used logfile of the station
 * @param client handle of the IPC-connection to the station
 */
void kr_logname (kr_client_t *client);

/** @} */

/** @name Remote Control Functions
 * Functions to enable or disable remote control interfaces
 * @{
 */

void kr_remote_status (kr_client_t *client);

/**
 * @brief Enable IPC remote control on a specifed port. You can use 
 * "hostname:port" as sysname to specify this station from another machine. 
 * This can only be enabled once per station.
 * @param client handle of the IPC-connection to the station
 * @param port TCP-port on which to listen for incoming connections
 */
void kr_remote_enable (kr_client_t *client, char *interface, int port);

/**
 * @brief Disable the previously enabled IPC remote control
 * @param client handle of the IPC-connection to the station
 */
void kr_remote_disable (kr_client_t *client, char *interface, int port);

/**
 * @brief Enable web UI remote control on a specifed port. This can only be 
 * enabled once per station.
 * @param client handle of the IPC-connection to the station
 * @param http_port port on which to listen for incoming HTTP-connections
 * @param websocket_port port used for communication between the web-page
 * and the station. Must not be the same as http_port!
 * @param headcode override the header delivered to HTTP-clients
 * @param header override the header delivered to HTTP-clients
 * @param footer override the footer delivered to HTTP-clients
 */
void kr_web_enable (kr_client_t *client, int http_port, int websocket_port, char *headcode, char *header, char *footer);

/**
 * @brief Disable the previously enabled web UI remote control
 * @param client handle of the IPC-connection to the station
 */
void kr_web_disable (kr_client_t *client);

/**
 * @brief Enable Open Sound Control (OSC) remote control on a specifed port.
 * This can only be enabled once per station.
 * @param client handle of the IPC-connection to the station
 * @param port port on which to listen for incoming connections
 */
void kr_osc_enable (kr_client_t *client, int port);

/**
 * @brief Disable the previously enabled OSC remote control
 * @param client handle of the IPC-connection to the station
 */
void kr_osc_disable (kr_client_t *client);

/** @} */

/** @name Tag Control Functions
 * Functions to set and read tags
 * @{
 */

/**
 * @todo oneman document this! kripton has no clue here!
 * @brief kr_read_tag_inner
 * @param client
 * @param tag_item
 * @param tag_name
 * @param tag_value
 */
void kr_read_tag_inner ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value );

/**
 * @todo oneman document this! kripton has no clue here!
 * @brief kr_read_tag
 * @param client
 * @param tag_item
 * @param tag_name
 * @param tag_value
 * @return 
 */
int kr_read_tag ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value );

/**
 * @brief prints out a list of all tags in a group
 * @param client handle of the IPC-connection to the station
 * @param item item to print the tags of
 */
void kr_tags (kr_client_t *client, char *item);

/**
 * @brief prints out the value of one specified tag
 * @param client handle of the IPC-connection to the station
 * @param item item the tag is grouped under
 * @param tag_name name of the tag to be printed
 */
void kr_tag (kr_client_t *client, char *item, char *tag_name);

/**
 * @brief sets the value for a specified tag
 * @param client handle of the IPC-connection to the station
 * @param item item the tag is grouped under
 * @param tag_name name of the tag to be set
 * @param tag_value value to set the tag to
 */
void kr_set_tag (kr_client_t *client, char *item, char *tag_name, char *tag_value);

/** @} */

/** @} */

#endif
