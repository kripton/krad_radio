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

/**
 * @brief connect to a krad radio daemon identified by sysname
 * @param sysname of local station or ip:port remote station
 * @return connection handle or NULL on error
 */
kr_client_t *kr_connect (char *sysname);

/**
 * @brief disconnect an open IPC-connection
 * @param kr_client pointer to handle of the connection to be closed
 */
void kr_disconnect (kr_client_t **kr_client);

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

/**
 * @brief prints out the type of a message
 * @param kr_client handle of the IPC-connection to the station
 */
void kr_print_message_type (kr_client_t *kr_client);

/**
 * @brief prints out the client's response to a request-message
 * @param kr_client handle of the IPC-connection to the station
 */
void kr_client_print_response (kr_client_t *kr_client);

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

/**
 * @brief Enable IPC remote control on a specifed port. You can use 
 * "hostname:port" as sysname to specify this station from another machine. 
 * This can only be enabled once per station.
 * @param client handle of the IPC-connection to the station
 * @param port TCP-port on which to listen for incoming connections
 */
void kr_remote_enable (kr_client_t *client, int port);

/**
 * @brief Disable the previously enabled IPC remote control
 * @param client handle of the IPC-connection to the station
 */
void kr_remote_disable (kr_client_t *client);

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
