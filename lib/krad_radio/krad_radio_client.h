//#ifndef KRAD_RADIO_CLIENT_H
//#define KRAD_RADIO_CLIENT_H
//#define KRAD_RADIO_CLIENT 1

#include <inttypes.h>


typedef struct kr_client_St kr_client_t;
typedef struct kr_shm_St kr_shm_t;

kr_client_t *kr_connect (char *sysname);
void kr_disconnect (kr_client_t **kr_client);
int kr_client_local (kr_client_t *kr_client);

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

//#endif
