#ifndef SYNTH3X_PROTOCOLS_H
#define SYNTH3X_PROTOCOLS_H

#include "compositor.h"

void protocols_init(compositor_t *c);
void protocols_register_globals(wl_client_t *client, uint32_t registry_id);
void protocols_bind(wl_client_t *client, uint32_t name, uint32_t id,
                    const char *iface, uint32_t version);
int  protocols_dispatch(wl_client_t *client, wl_object_t *obj,
                        uint32_t opcode, arg_reader_t *r);
void protocols_cleanup_client(compositor_t *c, wl_client_t *client);

#endif
