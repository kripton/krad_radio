/**
 * @file krad_radio_client.h
 * @brief Krad Radio Client API
 */

/**
 * @mainpage Krad Radio Client
 *
 * Krad Radio Client (Kripton this is where you come in)
 *
 *
 * Documentation sections:
 * @li @ref krad_radio_client_ctl
 * @li @ref krad_radio_client
 * @li @ref krad_mixer_client
 * @li @ref krad_compositor_client
 * @li @ref krad_transponder_client
 *
 * 
 */


#include <inttypes.h>


#define ALL_BROADCASTS 1


/** @defgroup krad_radio_client Krad Radio Client
  @{
  */

/** Krad Radio Client connection handle.
  * This is ...
  * @see kr_connect,kr_disconnect
  */
typedef struct kr_client_St kr_client_t;

typedef struct kr_shm_St kr_shm_t;

/**
 * Connect to a Krad Radio Daemon
 *
 * It can do this.
 * I've tried it.
 *
 * @param  sysname of local station or ip:port remote station
 * @return KR Client connection handle or NULL on fail
 */
kr_client_t *kr_connect (char *sysname);

/**
 * Disconnect from a Krad Radio Daemon
 *
 * Also frees a lil memory
 *
 * @param  KR Client connection handle pointer pointer
 * @return void
 */
void kr_disconnect (kr_client_t **kr_client);

/**
 * Is this connection local?
 *
 * Incase you forgot.
 *
 * @param  KR Client connection handle pointer
 * @return 1 if local 0 if not
 */
int kr_client_local (kr_client_t *kr_client);

void kr_broadcast_subscribe (kr_client_t *kr_client, uint32_t broadcast_id);
int kr_poll (kr_client_t *kr_client, uint32_t timeout_ms);

void kr_print_message_type (kr_client_t *kr_client);

void kr_client_print_response (kr_client_t *kr_client);

/** SHM for local A/V ports **/
kr_shm_t *kr_shm_create (kr_client_t *client);
void kr_shm_destroy (kr_shm_t *kr_shm);

/* Info */
void kr_uptime (kr_client_t *client);
void kr_system_info (kr_client_t *client);
void kr_system_cpu_usage (kr_client_t *client);
void kr_set_dir (kr_client_t *client, char *dir);
void kr_logname (kr_client_t *client);

/* Remote Control */
void kr_remote_enable (kr_client_t *client, int port);
void kr_remote_disable (kr_client_t *client);
void kr_web_enable (kr_client_t *client, int http_port, int websocket_port, char *headcode, char *header, char *footer);
void kr_web_disable (kr_client_t *client);
void kr_osc_enable (kr_client_t *client, int port);
void kr_osc_disable (kr_client_t *client);

/* Tagging */
void kr_read_tag_inner ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value );
int kr_read_tag ( kr_client_t *client, char **tag_item, char **tag_name, char **tag_value );

void kr_tags (kr_client_t *client, char *item);
void kr_tag (kr_client_t *client, char *item, char *tag_name);
void kr_set_tag (kr_client_t *client, char *item, char *tag_name, char *tag_value);

/**@}*/
