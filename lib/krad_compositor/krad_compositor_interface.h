#include "krad_compositor.h"

void krad_compositor_subunit_to_ebml (krad_ipc_server_t *krad_ipc, krad_compositor_subunit_t *krad_compositor_subunit);
void krad_compositor_sprite_to_ebml ( krad_ipc_server_t *krad_ipc, krad_sprite_t *krad_sprite, int number);
void krad_compositor_port_to_ebml ( krad_ipc_server_t *krad_ipc, krad_compositor_port_t *krad_compositor_port);
void krad_compositor_text_to_ebml ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc, krad_text_t *krad_text, int number);
int krad_compositor_handler ( krad_compositor_t *krad_compositor, krad_ipc_server_t *krad_ipc );
