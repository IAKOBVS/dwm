#ifndef XCB_RES_H
#define XCB_RES_H
#include <xcb/xcb.h>
/* Resource query stubs for dwm */
typedef struct { unsigned int sequence; unsigned int length; } xcb_res_query_client_ids_cookie_t;
typedef struct { unsigned int sequence; unsigned int length; } xcb_res_query_client_ids_reply_t;
typedef struct { uint32_t client; uint32_t mask; } xcb_res_client_id_spec_t;
#define XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID 1
typedef struct { xcb_res_client_id_spec_t spec; uint32_t length; } xcb_res_client_id_value_t;
typedef struct { unsigned int rem; xcb_res_client_id_value_t *data; } xcb_res_client_id_value_iterator_t;

xcb_res_query_client_ids_cookie_t xcb_res_query_client_ids(xcb_connection_t *c, uint32_t spec_len, const void *spec);
xcb_res_query_client_ids_reply_t *xcb_res_query_client_ids_reply(xcb_connection_t *c, xcb_res_query_client_ids_cookie_t cookie, xcb_generic_error_t **e);
xcb_res_client_id_value_iterator_t xcb_res_query_client_ids_ids_iterator(void *r);
void xcb_res_client_id_value_next(xcb_res_client_id_value_iterator_t *i);
uint32_t *xcb_res_client_id_value_value(void *data);
void free(void *ptr);
#endif
